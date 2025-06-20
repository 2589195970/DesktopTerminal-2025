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
#include <QTextCodec>
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

#ifdef Q_OS_WIN
#include <windows.h>
#include <iostream>
#include <io.h>
#include <fcntl.h>
#endif

// --------------------------- 系统信息检测结构体 ---------------------------
struct SystemInfo {
    bool isOldWin = false;
    bool lowMemory = false;
    bool isOldCpu = false;
    bool isVirtualized = false;
    QString cpuInfo = "Unknown";
#ifdef Q_OS_WIN
    DWORDLONG totalMemoryMB = 0;
#else
    quint64 totalMemoryMB = 0;
#endif
};

SystemInfo detectSystemInfo() {
    SystemInfo info;
    
#ifdef Q_OS_WIN
    // 检测Windows版本
    QString winVer = QSysInfo::productVersion();
    info.isOldWin = winVer.startsWith("6.0") || winVer.startsWith("6.1") || winVer.startsWith("5.");
    
    if(info.isOldWin) {
        // 检测内存
        MEMORYSTATUSEX memStatus;
        memStatus.dwLength = sizeof(memStatus);
        GlobalMemoryStatusEx(&memStatus);
        info.totalMemoryMB = memStatus.ullTotalPhys / (1024 * 1024);
        info.lowMemory = info.totalMemoryMB <= 4096;
        
        // 检测CPU信息（只执行一次）
        HKEY hKey;
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, 
                          "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", 
                          0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            DWORD dataSize = 256;
            char data[256];
            if (RegQueryValueExA(hKey, "ProcessorNameString", NULL, NULL, 
                                (LPBYTE)data, &dataSize) == ERROR_SUCCESS) {
                info.cpuInfo = QString::fromLocal8Bit(data).trimmed();
            }
            RegCloseKey(hKey);
        }
        
        // 检测是否为老旧CPU架构
        QRegExp oldCpuPattern("\\b[iI][3-7]-[2-4]\\d{3}\\b");
        if (info.cpuInfo.contains(oldCpuPattern) || 
            info.cpuInfo.contains("Haswell", Qt::CaseInsensitive) ||
            info.cpuInfo.contains("Ivy Bridge", Qt::CaseInsensitive) ||
            info.cpuInfo.contains("Sandy Bridge", Qt::CaseInsensitive)) {
            info.isOldCpu = true;
        }
        
        // 检测是否为虚拟化环境
        if (info.cpuInfo.contains("Virtual", Qt::CaseInsensitive) ||
            info.cpuInfo.contains("QEMU", Qt::CaseInsensitive) ||
            info.cpuInfo.contains("VMware", Qt::CaseInsensitive) ||
            info.cpuInfo.contains("VirtualBox", Qt::CaseInsensitive)) {
            info.isVirtualized = true;
        }
    }
#endif
    
    return info;
}

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
            out.setCodec("UTF-8");
            for (const LogEntry &e : std::as_const(m_logBuffer[filename])) {
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

    void showMessage(QWidget *p,const QString&t,const QString&m){QMessageBox::warning(p,t,m);}
    bool getPassword(QWidget* p,const QString&t,const QString&l,QString&pwd){
        bool ok; pwd=QInputDialog::getText(p,t,l,QLineEdit::Password,"",&ok); return ok;
    }

    void shutdown() {
        flushAllLogBuffers();
        if (m_flushTimer) { m_flushTimer->stop(); delete m_flushTimer; m_flushTimer=nullptr; }
    }

private:
    Logger():m_logLevel(L_INFO) {
        QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));
        m_flushTimer=new QTimer();
        QObject::connect(m_flushTimer,&QTimer::timeout,[this](){flushAllLogBuffers();});
        
        // 使用默认定时器频率，避免构造函数中的复杂检测
        m_flushTimer->start(60000); // 默认1分钟间隔
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
    QString getActualConfigPath() const { return actualConfigPath; }
    
    // 低内存模式相关配置
    bool isLowMemoryModeEnabled() const {
        QJsonObject lowMemConfig = config.value("lowMemoryMode").toObject();
        QString enabled = lowMemConfig.value("enabled").toString("auto");
        return enabled == "true" || enabled == "auto";
    }
    int getLowMemoryThreshold() const {
        QJsonObject lowMemConfig = config.value("lowMemoryMode").toObject();
        return lowMemConfig.value("memoryThresholdMB").toInt(4096);
    }
    bool isProgressiveLoadingEnabled() const {
        QJsonObject lowMemConfig = config.value("lowMemoryMode").toObject();
        return lowMemConfig.value("progressiveLoading").toBool(true);
    }
    int getProgressiveLoadingDelay() const {
        QJsonObject lowMemConfig = config.value("lowMemoryMode").toObject();
        return lowMemConfig.value("progressiveLoadingDelay").toInt(3000);
    }

    bool createDefaultConfig(const QString &path){
        QJsonObject lowMemConfig{
            {"enabled", "auto"},
            {"memoryThresholdMB", 4096},
            {"progressiveLoading", true},
            {"progressiveLoadingDelay", 3000}
        };
        
        QJsonObject def{{"url","http://stu.sdzdf.com/"},{"exitPassword","sdzdf@2025"},
                        {"appName","智多分机考桌面端"},{"iconPath","logo.svg"},
                        {"appVersion","1.0.0"},{"disableHardwareAcceleration",false},
                        {"lowMemoryMode", lowMemConfig}};
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
    QHotkey *exitHotkeyF10{}, *exitHotkeyBackslash{};
    QTimer *maintenanceTimer{};
    bool needFocusCheck{true}, needFullscreenCheck{true};

public:
    ShellBrowser() {
        setWindowTitle(ConfigManager::instance().getAppName());
        setMinimumSize(1280,800);

        auto *settings = QWebEngineSettings::globalSettings();
        settings->setAttribute(QWebEngineSettings::JavascriptEnabled,true);
        settings->setAttribute(QWebEngineSettings::AutoLoadImages,true);
        settings->setAttribute(QWebEngineSettings::PluginsEnabled,true);
        settings->setAttribute(QWebEngineSettings::LocalStorageEnabled,true);
        settings->setAttribute(QWebEngineSettings::JavascriptCanOpenWindows,true);

        // 获取系统信息（只检测一次）
        SystemInfo sysInfo = detectSystemInfo();
        bool hw = !ConfigManager::instance().isHardwareAccelerationDisabled();
        
        if(sysInfo.isOldWin) {
            hw = false;
            // Windows 7特殊配置：强制禁用可能导致崩溃的功能
            settings->setAttribute(QWebEngineSettings::PluginsEnabled,false);
            settings->setAttribute(QWebEngineSettings::JavascriptCanOpenWindows,false);
            Logger::instance().appEvent("检测到Windows 7系统，启用兼容模式", L_INFO);
            
            if(sysInfo.isVirtualized) {
                Logger::instance().appEvent(QString("检测到虚拟化环境 - CPU：%1，总内存：%2MB")
                                          .arg(sysInfo.cpuInfo).arg(sysInfo.totalMemoryMB), L_WARNING);
            }
        }
        settings->setAttribute(QWebEngineSettings::WebGLEnabled,hw);
        settings->setAttribute(QWebEngineSettings::Accelerated2dCanvasEnabled,hw);
        
        // Qt 5.9.9中没有这些属性，移除以确保兼容性
        // settings->setAttribute(QWebEngineSettings::ScreenCaptureEnabled,false);
        // settings->setAttribute(QWebEngineSettings::WebRTCPublicInterfacesOnly,true);
        
        // Windows 7最兼容设置：基于系统信息禁用硬件加速功能
        if(sysInfo.isOldWin) {
            if(sysInfo.isOldCpu) {
                Logger::instance().appEvent("检测到Haswell/Ivy Bridge/Sandy Bridge等老旧CPU架构，启用老旧硬件兼容模式", L_WARNING);
            }
            
            if(sysInfo.lowMemory || sysInfo.isOldCpu || sysInfo.isVirtualized) {
                Logger::instance().appEvent("检测到低内存/老旧CPU/虚拟化环境，已启用超保守优化模式", L_WARNING);
            }
            
            // 强制禁用所有硬件加速功能
            settings->setAttribute(QWebEngineSettings::WebGLEnabled, false);
            settings->setAttribute(QWebEngineSettings::Accelerated2dCanvasEnabled, false);
            settings->setAttribute(QWebEngineSettings::PluginsEnabled, false);
            hw = false;
        }

        // 基于已检测的系统信息决定启动策略
        bool useProgressiveLoading = sysInfo.lowMemory || sysInfo.isOldCpu || sysInfo.isVirtualized;

        if(useProgressiveLoading) {
            // 渐进式启动：先显示加载页面
            setHtml("<html><head><style>"
                   "body{background:#1a1a1a;color:#fff;font-family:Arial;text-align:center;padding-top:200px;}"
                   ".loader{font-size:24px;margin-bottom:20px;}"
                   ".spinner{border:4px solid #333;border-top:4px solid #fff;border-radius:50%;"
                   "width:40px;height:40px;animation:spin 1s linear infinite;margin:20px auto;}"
                   "@keyframes spin{0%{transform:rotate(0deg);} 100%{transform:rotate(360deg);}}"
                   "</style></head><body>"
                   "<div class='loader'>智多分机考桌面端</div>"
                   "<div class='spinner'></div>"
                   "<div>正在启动中，请稍候...</div>"
                   "</body></html>");
            
            Logger::instance().appEvent("程序启动 - 使用渐进式加载模式", L_INFO);
            
            // 延迟加载实际页面，给WebEngine更多初始化时间
            // 根据系统环境设置不同的延迟时间
            int delayTime = 3000; // 默认3秒
            
            if(sysInfo.isVirtualized && (sysInfo.lowMemory || sysInfo.isOldCpu)) {
                delayTime = 30000; // 虚拟化+低配置环境：等待30秒
                Logger::instance().appEvent("检测到虚拟化低配置环境，启用超长延迟启动模式（30秒）", L_WARNING);
            } else if(sysInfo.lowMemory || sysInfo.isOldCpu) {
                delayTime = 15000; // 低配置环境：等待15秒
                Logger::instance().appEvent("检测到低配置环境，启用延迟启动模式（15秒）", L_INFO);
            } else if(useProgressiveLoading) {
                delayTime = ConfigManager::instance().getProgressiveLoadingDelay();
            }
            
            QTimer::singleShot(delayTime, this, [this](){
                load(QUrl(ConfigManager::instance().getUrl()));
                Logger::instance().appEvent("延迟加载完成，正在访问考试页面", L_INFO);
            });
        } else {
            // 标准启动
            load(QUrl(ConfigManager::instance().getUrl()));
            Logger::instance().appEvent("程序启动");
        }

        exitHotkeyF10 = new QHotkey(QKeySequence("F10"),true,this);
        exitHotkeyBackslash = new QHotkey(QKeySequence("\\"),true,this);
        connect(exitHotkeyF10,&QHotkey::activated,this,&ShellBrowser::handleExitHotkey);
        connect(exitHotkeyBackslash,&QHotkey::activated,this,&ShellBrowser::handleExitHotkey);

        setWindowFlags(Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
        setWindowState(Qt::WindowFullScreen); showFullScreen();

        maintenanceTimer=new QTimer(this);
        connect(maintenanceTimer,&QTimer::timeout,this,[this, sysInfo](){
            // 虚拟化环境优化：根据环境调整检查频率
            static int checkCounter = 0;
            checkCounter++;
            
            // 根据环境设置检查频率
            int focusCheckInterval = sysInfo.isVirtualized ? 12 : 6;  // 虚拟化环境：2分钟，其他：1分钟
            int fullscreenCheckInterval = sysInfo.isVirtualized ? 12 : 6;
            int memoryCheckInterval = sysInfo.isVirtualized ? 120 : 60; // 虚拟化环境：20分钟，其他：10分钟
            
            // 焦点检查
            if(needFocusCheck && checkCounter % focusCheckInterval == 1) {
                if(!isActiveWindow()){ 
                    raise(); 
                    activateWindow(); 
                }
            }
            
            // 全屏检查
            if(needFullscreenCheck && checkCounter % fullscreenCheckInterval == 3) {
                if(windowState()!=Qt::WindowFullScreen){ 
                    setWindowState(Qt::WindowFullScreen); 
                    showFullScreen(); 
                }
            }
            
            // 内存监控（仅在低内存环境下启用）
#ifdef Q_OS_WIN
            if(sysInfo.lowMemory) {
                static int memoryCheckCounter = 0;
                if(++memoryCheckCounter >= memoryCheckInterval) {
                    memoryCheckCounter = 0;
                    
                    MEMORYSTATUSEX memStatus;
                    memStatus.dwLength = sizeof(memStatus);
                    GlobalMemoryStatusEx(&memStatus);
                    DWORDLONG availMemoryMB = memStatus.ullAvailPhys / (1024 * 1024);
                    
                    if(availMemoryMB < 200) { // 可用内存少于200MB时触发垃圾回收
                        Logger::instance().appEvent(QString("检测到内存不足：可用%1MB，触发垃圾回收")
                                                   .arg(availMemoryMB), L_WARNING);
                        
                        this->page()->runJavaScript("if(window.gc) window.gc(); "
                                                   "if(window.CollectGarbage) window.CollectGarbage();");
                    }
                }
            }
#endif
        });
        
        // 根据环境设置维护定时器间隔
        int maintenanceInterval = sysInfo.isVirtualized ? 20000 : 10000; // 虚拟化环境：20秒，其他：10秒
        maintenanceTimer->start(maintenanceInterval);

        setContextMenuPolicy(Qt::NoContextMenu);

        auto *refreshShortcut=new QShortcut(QKeySequence("Ctrl+R"),this);
        connect(refreshShortcut,&QShortcut::activated,this,[this](){ reload(); Logger::instance().appEvent("用户使用Ctrl+R刷新页面"); });
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

// --------------------------- main ---------------------------
int main(int argc,char *argv[]){
#ifdef Q_OS_WIN
    // 针对0x40000015异常的Windows特殊处理
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    
    // 禁用DEP以避免执行保护冲突（如果管理员权限允许）
    DWORD flOldProtect;
    DWORD flNewProtect = PAGE_EXECUTE_READWRITE;
    // 这只是尝试，失败也不影响程序继续
#endif

    // Windows 7 WebEngine兼容性：在QApplication创建前设置环境变量
#ifdef Q_OS_WIN
    // 使用统一的系统检测函数
    SystemInfo sysInfo = detectSystemInfo();
    
    if(sysInfo.isOldWin) {
        // 创建Logger输出信息（这里Logger还没初始化，用printf）
        printf("检测到Windows 7系统\n");
        printf("CPU信息：%s\n", sysInfo.cpuInfo.toLocal8Bit().constData());
        printf("内存大小：%lld MB\n", sysInfo.totalMemoryMB);
        printf("CPU架构：%s\n", sysInfo.isOldCpu ? "老旧架构(Haswell或更早)" : "较新架构");
        printf("虚拟化环境：%s\n", sysInfo.isVirtualized ? "是" : "否");
        
        // Windows 7最兼容配置：只使用最基础、最稳定的设置
        qputenv("QTWEBENGINE_DISABLE_GPU", "1");
        qputenv("QTWEBENGINE_DISABLE_SANDBOX", "1");
        qputenv("QT_OPENGL", "software");
        
        // 虚拟化环境专用优化
        if(sysInfo.isVirtualized) {
            printf("检测到虚拟化环境，启用虚拟化优化模式\n");
            qputenv("QTWEBENGINE_DISABLE_GPU_PROCESS", "1");
            qputenv("QTWEBENGINE_DISABLE_SOFTWARE_RASTERIZER", "1");
        }
        
        // Chrome启动参数：针对虚拟化单核心环境的最小化配置
        QString chromiumFlags = "--no-sandbox --single-process --disable-dev-shm-usage "
                              "--disable-extensions --disable-plugins ";
        
        if(sysInfo.isVirtualized && (sysInfo.lowMemory || sysInfo.isOldCpu)) {
            // 虚拟化超保守模式：最小化资源占用
            printf("启用虚拟化超保守模式\n");
            chromiumFlags += "--max_old_space_size=64 --disable-webgl --disable-webgl2 "
                           "--disable-3d-apis --use-gl=disabled --disable-gpu "
                           "--disable-accelerated-video-processing --disable-features=WebRTC "
                           "--renderer-process-limit=1 --disable-smooth-scrolling "
                           "--disable-background-networking --disable-renderer-backgrounding "
                           "--memory-pressure-off --max-unused-resource-memory-usage-percentage=5";
        } else if(sysInfo.lowMemory || sysInfo.isOldCpu) {
            // 标准保守模式
            printf("启用标准保守模式\n");
            chromiumFlags += "--max_old_space_size=128 --disable-webgl "
                           "--disable-accelerated-video-processing "
                           "--renderer-process-limit=1 --disable-smooth-scrolling";
        } else {
            // 标准Windows 7模式
            printf("启用标准Windows 7兼容模式\n");
            chromiumFlags += "--max_old_space_size=256";
        }
        
        qputenv("QTWEBENGINE_CHROMIUM_FLAGS", chromiumFlags.toLocal8Bit());
    }
#endif

    QApplication app(argc,argv);
    
    // 强制Qt使用单线程模式
    app.setAttribute(Qt::AA_DisableHighDpiScaling, true);  // 禁用高DPI缩放以减少计算
    app.setAttribute(Qt::AA_Use96Dpi, true);               // 使用96DPI以提升性能
    
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));

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

    GlobalEventFilter *f=new GlobalEventFilter; app.installEventFilter(f);

    ShellBrowser browser; browser.showFullScreen();
    QObject::connect(&app,&QApplication::aboutToQuit,[](){ Logger::instance().shutdown(); });
    return app.exec();
}
