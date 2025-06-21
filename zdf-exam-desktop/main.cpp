#include <QApplication>
#include <QWebEngineView>
#include <QWebEngineSettings>
#include <QWebEnginePage>
#include <QWebEngineProfile>
#include <QKeyEvent>
#include <QInputDialog>
#include <QMessageBox>
#include <QTimer>
#include <QWindow>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QDateTime>
#include <QHotkey>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QTextCodec>
#else
#include <QStringConverter>
#endif
#include <QMenu>
#include <QContextMenuEvent>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDir>
#include <QDebug>
#include <QStandardPaths>
#include <QWindowStateChangeEvent>
#include <QShortcut>
#include <QSysInfo>
#include <QVersionNumber>

#ifdef Q_OS_WIN
#include <windows.h>
#include <versionhelpers.h>
#include <iostream>
#include <io.h>
#include <fcntl.h>
#endif

// --------------------------- 日志相关 ---------------------------
enum LogLevel { L_DEBUG, L_INFO, L_WARNING, L_ERROR };

struct LogEntry {
    QDateTime timestamp;
    QString category;
    QString message;
    QString filename;
};

class Logger {
public:
    static Logger& instance() {
        static Logger logger;
        return logger;
    }

    void setLogLevel(LogLevel level) { m_logLevel = level; }
    LogLevel getLogLevel() const { return m_logLevel; }

    bool ensureLogDirectoryExists() {
        QString logDir = QCoreApplication::applicationDirPath() + "/log";
        QDir dir(logDir);
        if (!dir.exists())
            return dir.mkpath(".");
        return true;
    }

    void logEvent(const QString &category, const QString &message,
                  const QString &filename = "app.log", LogLevel level = L_INFO) {
        if (level < m_logLevel) return;

        LogEntry entry{QDateTime::currentDateTime(), category, message, filename};
        m_logBuffer[filename].append(entry);
        if (m_logBuffer[filename].size() >= LOG_BUFFER_SIZE || level >= L_WARNING)
            flushLogBuffer(filename);
    }

    void flushLogBuffer(const QString &filename) {
        if (m_logBuffer[filename].isEmpty()) return;
        if (!ensureLogDirectoryExists()) return;

        QString logPath = QCoreApplication::applicationDirPath() + "/log/" + filename;
        QFile file(logPath);
        if (file.open(QIODevice::Append | QIODevice::Text)) {
            QTextStream out(&file);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
            out.setCodec("UTF-8");
#else
            out.setEncoding(QStringConverter::Utf8);
#endif
            // 兼容C++11/14编译器，不使用std::as_const
            const auto& logBuffer = m_logBuffer[filename];
            for (const LogEntry &e : logBuffer) {
                out << e.timestamp.toString("yyyy-MM-dd hh:mm:ss")
                    << " | " << e.category << " | " << e.message << "\n";
            }
            file.close();
            m_logBuffer[filename].clear();
        }
    }

    void flushAllLogBuffers() {
        const QStringList keys = m_logBuffer.keys();
        for (const QString &k : keys) flushLogBuffer(k);
    }

    void appEvent(const QString &msg, LogLevel lv=L_INFO) { logEvent("应用程序", msg, "app.log", lv); }
    void configEvent(const QString &msg, LogLevel lv=L_INFO) { logEvent("配置文件", msg, "config.log", lv); }
    void hotkeyEvent(const QString &msg) { logEvent("热键退出尝试", msg, "exit.log", L_INFO); }
    void logStartup(const QString &path) { logEvent("启动", QString("程序启动成功，使用配置文件: %1").arg(path), "startup.log", L_INFO); }
    void errorEvent(const QString &msg, LogLevel lv=L_ERROR) { logEvent("错误", msg, "error.log", lv); }
    void systemEvent(const QString &msg, LogLevel lv=L_INFO) { logEvent("系统信息", msg, "system.log", lv); }

    void showMessage(QWidget *p,const QString&t,const QString&m){QMessageBox::warning(p,t,m);}
    void showCriticalError(QWidget *p,const QString&t,const QString&m){
        QMessageBox::critical(p,t,m);
        errorEvent(QString("%1: %2").arg(t).arg(m), L_ERROR);
    }
    bool getPassword(QWidget* p,const QString&t,const QString&l,QString&pwd){
        bool ok; 
        pwd=QInputDialog::getText(p,t,l,QLineEdit::Password,"",&ok); 
        return ok;
    }
    
    // 系统信息收集（用于问题诊断）
    void collectSystemInfo() {
        systemEvent(QString("Qt版本: %1").arg(QT_VERSION_STR));
        systemEvent(QString("操作系统: %1 %2").arg(QSysInfo::productType()).arg(QSysInfo::productVersion()));
        systemEvent(QString("系统架构: %1").arg(QSysInfo::currentCpuArchitecture()));
        systemEvent(QString("内核版本: %1 %2").arg(QSysInfo::kernelType()).arg(QSysInfo::kernelVersion()));
        systemEvent(QString("机器主机名: %1").arg(QSysInfo::machineHostName()));
        
        // GPU和显卡信息
        systemEvent(QString("OpenGL环境变量: QT_OPENGL=%1").arg(qgetenv("QT_OPENGL").constData()));
        systemEvent(QString("WebEngine标志: %1").arg(qgetenv("QTWEBENGINE_CHROMIUM_FLAGS").constData()));
        
        // 内存信息
        systemEvent("程序启动完成，系统信息已收集");
    }

    void shutdown() {
        flushAllLogBuffers();
        if (m_flushTimer) { m_flushTimer->stop(); delete m_flushTimer; m_flushTimer=nullptr; }
    }

private:
    Logger():m_logLevel(L_INFO) {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));
#endif
        m_flushTimer=new QTimer();
        QObject::connect(m_flushTimer,&QTimer::timeout,[this](){flushAllLogBuffers();});
        m_flushTimer->start(5000);
    }
    Logger(const Logger&)=delete; Logger& operator=(const Logger&)=delete;
    ~Logger(){shutdown();}
    static const int LOG_BUFFER_SIZE = 10;
    QMap<QString,QList<LogEntry>> m_logBuffer;
    LogLevel m_logLevel;
    QTimer* m_flushTimer{};
};

// --------------------------- 配置管理 ---------------------------
class ConfigManager {
public:
    static ConfigManager& instance(){static ConfigManager cm; return cm;}

    bool loadConfig(const QString &configPath="resources/config.json"){
        QString exe = QCoreApplication::applicationDirPath();
        QStringList paths{
            exe+"/config.json",
            QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)+"/config.json",
#ifdef Q_OS_UNIX
            "/etc/zdf-exam-desktop/config.json",
#endif
            exe+"/"+configPath,
            exe+"/../"+configPath,
            configPath
        };
        if (QDir::isAbsolutePath(configPath)) paths << configPath;

        for (const QString &p: paths){
            QFile f(p); if(!f.exists()) continue;
            if(!f.open(QIODevice::ReadOnly)) continue;
            QJsonParseError e; auto doc=QJsonDocument::fromJson(f.readAll(),&e); f.close();
            if(doc.isNull()||!doc.isObject()) continue;
            config=doc.object();
            if(!validateConfig()) continue;
            actualConfigPath=p; return true;
        }
        return false;
    }

    bool validateConfig() const {
        for (const QString &k : {"url","exitPassword","appName"})
            if(!config.contains(k)||config[k].toString().isEmpty()) return false;
        return true;
    }

    QString getUrl()          const { return config.value("url").toString("http://stu.sdzdf.com/"); }
    QString getExitPassword() const { return config.value("exitPassword").toString("123456"); }
    QString getAppName()      const { return config.value("appName").toString("zdf-exam-desktop"); }
    bool    isHardwareAccelerationDisabled() const { return config.value("disableHardwareAcceleration").toBool(false); }
    int     getMaxMemoryMB()  const { return config.value("maxMemoryMB").toInt(512); }
    bool    isLowMemoryMode() const { return config.value("lowMemoryMode").toBool(false); }
    QString getProcessModel() const { return config.value("processModel").toString("process-per-site"); }
    QString getActualConfigPath() const { return actualConfigPath; }

    bool createDefaultConfig(const QString &path){
        QJsonObject def{{"url","http://stu.sdzdf.com/"},{"exitPassword","sdzdf@2025"},
                        {"appName","智多分机考桌面端"},{"iconPath","logo.svg"},
                        {"appVersion","1.0.0"},{"disableHardwareAcceleration",false},
                        {"maxMemoryMB",512},{"lowMemoryMode",false},
                        {"processModel","process-per-site"}};
        QFileInfo fi(path); QDir d=fi.dir(); if(!d.exists()&&!d.mkpath(".")) return false;
        QFile f(path); if(!f.open(QIODevice::WriteOnly)) return false;
        f.write(QJsonDocument(def).toJson()); f.close(); return true;
    }

    QJsonObject config;
private:
    ConfigManager(){ loadConfig(); }
    QString actualConfigPath;
};

// --------------------------- 浏览器封装 ---------------------------
class ShellBrowser : public QWebEngineView {
    Q_OBJECT  // 添加Q_OBJECT宏以支持信号和槽
    
private:
    QHotkey *exitHotkeyF10;
    QHotkey *exitHotkeyBackslash;
    QTimer *maintenanceTimer;
    bool needFocusCheck;
    bool needFullscreenCheck;

public:
    ShellBrowser(QWidget *parent = nullptr) : QWebEngineView(parent),
        exitHotkeyF10(nullptr),
        exitHotkeyBackslash(nullptr), 
        maintenanceTimer(nullptr),
        needFocusCheck(true),
        needFullscreenCheck(true) {
        setWindowTitle(ConfigManager::instance().getAppName());
        setMinimumSize(1280,800);

        // 尝试初始化WebEngine并捕获错误
        try {
            Logger::instance().appEvent("开始初始化QtWebEngine设置...");
            auto *settings = QWebEngineSettings::globalSettings();
            
            settings->setAttribute(QWebEngineSettings::JavascriptEnabled,true);
            settings->setAttribute(QWebEngineSettings::AutoLoadImages,true);
            settings->setAttribute(QWebEngineSettings::PluginsEnabled,true);
            settings->setAttribute(QWebEngineSettings::LocalStorageEnabled,true);
            settings->setAttribute(QWebEngineSettings::PlaybackRequiresUserGesture,false);
            settings->setAttribute(QWebEngineSettings::JavascriptCanOpenWindows,true);
            Logger::instance().appEvent("基础WebEngine设置完成");

            bool hw = !ConfigManager::instance().isHardwareAccelerationDisabled();
#ifdef Q_OS_WIN
            // 使用更精确的版本检测
            QString ver = QSysInfo::productVersion();
            QVersionNumber winVersion = QVersionNumber::fromString(ver);
            bool isWin7OrOlder = winVersion.majorVersion() < 6 || 
                               (winVersion.majorVersion() == 6 && winVersion.minorVersion() <= 1);
                               
            if(isWin7OrOlder) {
                hw = false;
                Logger::instance().appEvent("Windows 7兼容模式：开始应用特殊设置");
                
                // Windows 7特殊优化：禁用更多消耗性能的功能
                settings->setAttribute(QWebEngineSettings::WebGLEnabled, false);
                settings->setAttribute(QWebEngineSettings::Accelerated2dCanvasEnabled, false);
                settings->setAttribute(QWebEngineSettings::HyperlinkAuditingEnabled, false);
                settings->setAttribute(QWebEngineSettings::ErrorPageEnabled, false);
                settings->setAttribute(QWebEngineSettings::FocusOnNavigationEnabled, false);
                
                // 减少网络连接数以降低资源消耗
                settings->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, false);
                settings->setAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls, false);
                
                Logger::instance().appEvent("Windows 7兼容模式：已禁用额外功能以优化性能", L_INFO);
            } else {
                Logger::instance().appEvent("检测到较新Windows系统，使用标准WebEngine设置", L_INFO);
            }
#endif
            settings->setAttribute(QWebEngineSettings::WebGLEnabled,hw);
            settings->setAttribute(QWebEngineSettings::Accelerated2dCanvasEnabled,hw);
            settings->setAttribute(QWebEngineSettings::ScreenCaptureEnabled,false);
            settings->setAttribute(QWebEngineSettings::WebRTCPublicInterfacesOnly,true);
            Logger::instance().appEvent("所有WebEngine设置已完成");

            Logger::instance().appEvent("开始加载目标URL...");
            load(QUrl(ConfigManager::instance().getUrl()));
            Logger::instance().appEvent("URL加载请求已发送");
            
        } catch (const std::exception& e) {
            QString errorMsg = QString("WebEngine初始化失败: %1").arg(QString::fromLocal8Bit(e.what()));
            Logger::instance().errorEvent(errorMsg, L_ERROR);
            Logger::instance().collectSystemInfo(); // 收集系统信息以便诊断
            Logger::instance().showCriticalError(this, "致命错误", 
                errorMsg + "\n\n请检查error.log和system.log文件获取详细信息。");
            Logger::instance().shutdown(); // 确保日志被写入
            QApplication::quit();
            return;
        } catch (...) {
            QString errorMsg = "WebEngine初始化失败: 未知错误";
            Logger::instance().errorEvent(errorMsg, L_ERROR);
            Logger::instance().collectSystemInfo(); // 收集系统信息以便诊断
            Logger::instance().showCriticalError(this, "致命错误", 
                errorMsg + "\n\n这通常是硬件加速、GPU驱动或Qt版本兼容性问题。\n"
                "建议检查：\n1. 更新显卡驱动\n2. 在config.json中设置disableHardwareAcceleration=true\n"
                "3. 检查error.log和system.log文件获取详细信息。");
            Logger::instance().shutdown(); // 确保日志被写入
            QApplication::quit();
            return;
        }
        
        Logger::instance().appEvent("程序启动完成，开始设置热键...");

        // 使用智能指针或指定parent管理内存
        exitHotkeyF10 = new QHotkey(QKeySequence("F10"), true, this);
        exitHotkeyBackslash = new QHotkey(QKeySequence("\\"), true, this);
        
        // 使用Qt::QueuedConnection提高稳定性
        connect(exitHotkeyF10, &QHotkey::activated, this, &ShellBrowser::handleExitHotkey, Qt::QueuedConnection);
        connect(exitHotkeyBackslash, &QHotkey::activated, this, &ShellBrowser::handleExitHotkey, Qt::QueuedConnection);

        setWindowFlags(Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
        setWindowState(Qt::WindowFullScreen); showFullScreen();

        maintenanceTimer = new QTimer(this);
        connect(maintenanceTimer, &QTimer::timeout, this, [this](){
            if(needFocusCheck && !isActiveWindow()) { 
                raise(); 
                activateWindow(); 
            }
            if(needFullscreenCheck && windowState() != Qt::WindowFullScreen) { 
                setWindowState(Qt::WindowFullScreen); 
                showFullScreen(); 
            }
        }, Qt::QueuedConnection);
        maintenanceTimer->start(1500);

        setContextMenuPolicy(Qt::NoContextMenu);

        auto *refreshShortcut = new QShortcut(QKeySequence("Ctrl+R"), this);
        connect(refreshShortcut, &QShortcut::activated, this, [this](){
            reload(); 
            Logger::instance().appEvent("用户使用Ctrl+R刷新页面"); 
        }, Qt::QueuedConnection);
    }
    
    // 添加析构函数以确保资源清理
    ~ShellBrowser() {
        // Qt的parent-child机制会自动清理，但显式设置为null避免意外访问
        if (maintenanceTimer) {
            maintenanceTimer->stop();
            maintenanceTimer = nullptr;
        }
        exitHotkeyF10 = nullptr;
        exitHotkeyBackslash = nullptr;
    }

protected:
    // ---------- 关键修改：更细粒度拦截 ----------
    bool event(QEvent *e) override {
        if(e->type()==QEvent::ShortcutOverride){
            QKeyEvent *k=static_cast<QKeyEvent*>(e);

            const bool onlyShift = (k->modifiers()==Qt::ShiftModifier);
            const bool ctrlR = (k->key()==Qt::Key_R && k->modifiers()==Qt::ControlModifier);

            if(onlyShift || ctrlR)                   // 放行单纯 Shift* 和 Ctrl+R
                return QWebEngineView::event(e);

            const bool hasSysMod = k->modifiers() & (Qt::AltModifier|Qt::ControlModifier|Qt::MetaModifier);
            if(hasSysMod){                           // 其余带系统修饰键全部拦
                e->accept(); return true;
            }
        }
        return QWebEngineView::event(e);
    }

    void contextMenuEvent(QContextMenuEvent *e) override { e->ignore(); }

    void handleExitHotkey(){
        needFocusCheck=false;
        QString pwd; bool ok=Logger::instance().getPassword(this,"安全退出","请输入退出密码：",pwd);
        QString exitPwd=ConfigManager::instance().getExitPassword();
        if(ok && pwd==exitPwd){
            Logger::instance().hotkeyEvent("密码正确，退出");
            Logger::instance().shutdown();
            QApplication::quit();
        }else{
            Logger::instance().hotkeyEvent(ok?"密码错误":"取消输入");
            Logger::instance().showMessage(this,"错误", ok?"密码错误":"已取消");
            needFocusCheck=true;
        }
    }

    void closeEvent(QCloseEvent *e) override { e->ignore(); }
    void focusOutEvent(QFocusEvent *e) override { QWebEngineView::focusOutEvent(e); }

    void keyPressEvent(QKeyEvent *e) override {
        QString ks=QKeySequence(e->key()|e->modifiers()).toString();
        Logger::instance().appEvent(QString("按键事件: %1").arg(ks));

        if(e->key()==Qt::Key_R && e->modifiers()==Qt::ControlModifier){
            reload(); e->accept(); return;
        }
        if(e->key()==Qt::Key_Escape &&
           (e->modifiers()&(Qt::ControlModifier|Qt::AltModifier|Qt::MetaModifier))){
            e->ignore(); return;
        }
        QWebEngineView::keyPressEvent(e);
    }
};

// 包含MOC生成的代码（如果需要）
#include "main.moc"

// --------------------------- 全局事件过滤器 ---------------------------
class GlobalEventFilter : public QObject {
protected:
    bool eventFilter(QObject *obj,QEvent *ev) override {
        if(ev->type()==QEvent::KeyPress){
            QKeyEvent *k=static_cast<QKeyEvent*>(ev);

            const bool hasSysMod = k->modifiers() & (Qt::AltModifier|Qt::ControlModifier|Qt::MetaModifier);
            if(hasSysMod){
                if(k->key()==Qt::Key_R && k->modifiers()==Qt::ControlModifier)
                    return false;        // 允许 Ctrl+R
                ev->accept(); return true; // 其余带系统修饰符全部拦截
            }

            // 特定系统快捷键额外拦（例 Alt+Tab / Win+Tab）
            if( (k->key()==Qt::Key_Tab && (k->modifiers()&Qt::AltModifier)) ||
                (k->key()==Qt::Key_Tab && (k->modifiers()&Qt::MetaModifier)) ||
                (k->key()==Qt::Key_Delete && (k->modifiers()&(Qt::ControlModifier|Qt::AltModifier))) )
                { ev->accept(); return true; }
        }

        if(ev->type()==QEvent::WindowStateChange){
            auto *ws=static_cast<QWindowStateChangeEvent*>(ev);
            if(!(ws->oldState()&Qt::WindowFullScreen)){
                if(QWindow *w=qobject_cast<QWindow*>(obj)){
                    w->setWindowState(Qt::WindowFullScreen);
                    return true;
                }
            }
        }
        return QObject::eventFilter(obj,ev);
    }
};

// 全局异常处理函数
void handleFatalError(const QString &errorMsg) {
    // 紧急情况下直接写文件，不依赖Logger实例
    QString logPath = QCoreApplication::applicationDirPath() + "/log/crash.log";
    QDir logDir = QFileInfo(logPath).dir();
    if (!logDir.exists()) logDir.mkpath(".");
    
    QFile crashFile(logPath);
    if (crashFile.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&crashFile);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        out.setCodec("UTF-8");
#else
        out.setEncoding(QStringConverter::Utf8);
#endif
        out << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") 
            << " | 致命错误 | " << errorMsg << "\n";
        crashFile.close();
    }
    
    // 显示紧急弹窗（如果可能）
    QMessageBox::critical(nullptr, "程序崩溃", 
        QString("程序遇到致命错误需要退出：\n\n%1\n\n"
                "错误信息已保存到crash.log文件中。\n"
                "请联系技术支持并提供log文件夹中的所有日志文件。").arg(errorMsg));
}

// Windows API兼容性检查函数
#ifdef Q_OS_WIN
bool checkWindowsAPICompatibility() {
    // 检查是否支持CreateFile2 API（Windows 8+）
    HMODULE kernel32 = GetModuleHandleA("kernel32.dll");
    if (!kernel32) return false;
    
    // 检查关键API的可用性
    bool hasCreateFile2 = (GetProcAddress(kernel32, "CreateFile2") != nullptr);
    bool hasSetFileInformationByHandle = (GetProcAddress(kernel32, "SetFileInformationByHandle") != nullptr);
    
    return hasCreateFile2 && hasSetFileInformationByHandle;
}

// 获取准确的Windows版本信息
QString getDetailedWindowsVersion() {
    if (IsWindows10OrGreater()) return "10+";
    if (IsWindows8Point1OrGreater()) return "8.1";
    if (IsWindows8OrGreater()) return "8";
    if (IsWindows7SP1OrGreater()) return "7 SP1";
    if (IsWindows7OrGreater()) return "7";
    if (IsWindowsVistaOrGreater()) return "Vista";
    return "XP或更早";
}
#endif

// --------------------------- main ---------------------------
int main(int argc,char *argv[]){
    // ============ Windows版本检测和API兼容性验证 ============
    // 声明变量以确保在整个main函数中可访问
    bool isWin7OrOlder = false;
    bool hasAPICompatibility = true;
    QString detailedVersion;
    
#ifdef Q_OS_WIN
    QString winVer = QSysInfo::productVersion();
    QVersionNumber winVersion = QVersionNumber::fromString(winVer);
    isWin7OrOlder = winVersion.majorVersion() < 6 || 
                   (winVersion.majorVersion() == 6 && winVersion.minorVersion() <= 1);
    
    // 详细版本检测
    detailedVersion = getDetailedWindowsVersion();
    hasAPICompatibility = checkWindowsAPICompatibility();
    
    // 如果是Windows 7且缺少关键API，提前警告
    if (isWin7OrOlder && !hasAPICompatibility) {
        MessageBoxA(nullptr, 
            "检测到您使用的是Windows 7系统，但缺少程序运行所需的API。\n\n"
            "这通常是由于：\n"
            "1. 系统未安装最新的Windows更新\n"
            "2. 缺少必要的运行时库\n"
            "3. Qt WebEngine与当前系统不兼容\n\n"
            "建议：\n"
            "- 安装所有Windows更新\n"
            "- 安装Visual C++ 2019-2022运行时\n"
            "- 考虑升级到Windows 10或更高版本\n\n"
            "程序将尝试以兼容模式运行，但可能存在功能限制。",
            "兼容性警告", MB_OK | MB_ICONWARNING);
    }
#else
    detailedVersion = "非Windows系统";
#endif

    // ============ 关键修复：彻底禁用硬件加速解决黑屏和高CPU问题 ============
#ifdef Q_OS_WIN
    if (isWin7OrOlder) {
        // Windows 7及更老版本：彻底禁用GPU进程和硬件加速，并添加API兼容性标志
        QString chromiumFlags = "--disable-gpu "
                               "--disable-gpu-compositing "
                               "--disable-gpu-rasterization "
                               "--disable-gpu-sandbox "
                               "--disable-software-rasterizer "
                               "--disable-backgrounding-occluded-windows "
                               "--disable-renderer-backgrounding "
                               "--disable-background-timer-throttling "
                               "--disable-background-networking "
                               "--disable-default-apps "
                               "--disable-extensions "
                               "--disable-sync "
                               "--no-sandbox "
                               "--single-process "
                               "--disable-features=VizDisplayCompositor,VizHitTestSurfaceLayer "
                               "--disable-dev-shm-usage "
                               "--disable-web-security "
                               "--allow-running-insecure-content "
                               "--ignore-certificate-errors "
                               "--disable-logging "
                               "--log-level=3";
        
        // 针对API兼容性问题，添加更多兼容性标志
        if (!hasAPICompatibility) {
            chromiumFlags += " --disable-features=AudioServiceOutOfProcess,AudioServiceSandbox"
                           " --disable-ipc-flooding-protection"
                           " --disable-renderer-accessibility"
                           " --disable-win32k-lockdown"
                           " --disable-features=WinUseBrowserSpellChecker";
        }
        
        qputenv("QTWEBENGINE_CHROMIUM_FLAGS", chromiumFlags.toLocal8Bit());
        
        // 禁用GPU线程避免卡顿
        qputenv("QTWEBENGINE_DISABLE_GPU_THREAD", "1");
        
        // 强制软件渲染
        qputenv("QT_OPENGL", "software");
        qputenv("QT_ANGLE_PLATFORM", "d3d9");
        
        // 强制使用兼容性模式
        qputenv("QTWEBENGINE_CHROMIUM_FLAGS_DISABLE_GPU", "1");
        qputenv("QTWEBENGINE_REMOTE_DEBUGGING", "0");
        
        // 日志将在QApplication创建后记录
    } else {
        // 较新Windows版本：适度优化
        qputenv("QTWEBENGINE_CHROMIUM_FLAGS", 
                "--disable-background-timer-throttling "
                "--disable-renderer-backgrounding "
                "--max_old_space_size=512");
    }
#endif

    QApplication app(argc,argv);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));
#endif

#ifdef Q_OS_WIN
    SetConsoleOutputCP(CP_UTF8); SetConsoleCP(CP_UTF8);
#endif

    app.setApplicationName("DesktopTerminal"); app.setOrganizationName("智多分");
    app.setQuitOnLastWindowClosed(false);

#ifdef Q_OS_MAC
    app.setAttribute(Qt::AA_PluginApplication,true);
#endif

#ifdef QT_DEBUG
    Logger::instance().setLogLevel(L_DEBUG);
#else
    Logger::instance().setLogLevel(L_INFO);
#endif
    Logger::instance().ensureLogDirectoryExists();
    Logger::instance().appEvent("应用程序初始化...");
    
    // 收集系统信息用于问题诊断
    Logger::instance().collectSystemInfo();

#ifdef Q_OS_WIN
    // 重用前面定义的变量，避免重复定义
    if (isWin7OrOlder) {
        Logger::instance().appEvent("检测到Windows 7系统，已启用兼容模式（彻底禁用硬件加速）", L_WARNING);
    } else {
        Logger::instance().appEvent("检测到较新Windows系统，使用标准优化模式", L_INFO);
    }
#endif

    ConfigManager &cfg=ConfigManager::instance();
    if(!cfg.loadConfig()){
        QString p=QCoreApplication::applicationDirPath()+"/config.json";
        if(cfg.createDefaultConfig(p)&&cfg.loadConfig(p)){
            QMessageBox::information(nullptr,"提示",
                QString("已生成默认配置文件：\n%1\n请修改后重新启动。").arg(p));
        }else{
            QMessageBox::critical(nullptr,"错误","无法加载或创建配置文件，程序退出。");
            return 1;
        }
    }
    Logger::instance().logStartup(cfg.getActualConfigPath());

    // ============ 内存优化：根据配置动态调整 ============
    // 注意：在QApplication创建后修改环境变量可能无效，但保留以供调试
    if(cfg.isLowMemoryMode() || cfg.getMaxMemoryMB() <= 1024) {
        Logger::instance().appEvent(QString("检测到低内存模式配置，内存限制: %1MB").arg(cfg.getMaxMemoryMB()), L_WARNING);
        Logger::instance().appEvent("注意：部分内存优化需要在程序启动前设置环境变量", L_INFO);
    }

    GlobalEventFilter *f=new GlobalEventFilter; app.installEventFilter(f);

    // 包装主浏览器创建和运行逻辑，捕获任何未预期的异常
    try {
        ShellBrowser browser; 
        browser.showFullScreen();
        
        QObject::connect(&app,&QApplication::aboutToQuit,[](){ 
            Logger::instance().appEvent("程序正常退出");
            Logger::instance().shutdown(); 
        });
        
        Logger::instance().appEvent("程序完全启动成功，进入主循环");
        return app.exec();
        
    } catch (const std::exception& e) {
        QString errorMsg = QString("程序启动过程中发生标准异常: %1").arg(e.what());
        handleFatalError(errorMsg);
        return 1;
    } catch (...) {
        QString errorMsg;
        
#ifdef Q_OS_WIN
        if (isWin7OrOlder && !hasAPICompatibility) {
            errorMsg = QString("程序启动失败：Windows 7 API兼容性问题\n\n"
                      "您的系统缺少程序运行所需的API（CreateFile2等）。\n\n"
                      "解决方案：\n"
                      "1. 安装所有Windows 7更新（特别是KB2533623）\n"
                      "2. 安装Microsoft Visual C++ 2019-2022运行时\n"
                      "3. 安装.NET Framework 4.7.2或更高版本\n"
                      "4. 考虑升级到Windows 10/11以获得最佳兼容性\n\n"
                      "技术详情：Qt WebEngine需要Windows 8+的API，\n"
                      "但可以通过系统更新在Windows 7上运行。");
        } else {
            errorMsg = QString("程序启动过程中发生未知异常，这通常是由于:\n"
                              "1. QtWebEngine无法初始化（GPU/显卡驱动问题）\n"
                              "2. 系统资源不足（内存/CPU）\n"
                              "3. 虚拟机环境兼容性问题\n\n"
                              "建议检查：\n"
                              "- 更新显卡驱动\n"
                              "- 增加虚拟机内存分配\n"
                              "- 确保config.json中disableHardwareAcceleration设为true");
        }
#else
        errorMsg = QString("程序启动过程中发生未知异常，请检查系统兼容性");
#endif
        
        handleFatalError(errorMsg);
        return 1;
    }
}
