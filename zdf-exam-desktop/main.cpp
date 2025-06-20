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
    QString getActualConfigPath() const { return actualConfigPath; }

    bool createDefaultConfig(const QString &path){
        QJsonObject def{{"url","http://stu.sdzdf.com/"},{"exitPassword","sdzdf@2025"},
                        {"appName","智多分机考桌面端"},{"iconPath","logo.svg"},
                        {"appVersion","1.0.0"},{"disableHardwareAcceleration",false}};
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
        settings->setAttribute(QWebEngineSettings::PlaybackRequiresUserGesture,false);
        settings->setAttribute(QWebEngineSettings::JavascriptCanOpenWindows,true);

        bool hw=!ConfigManager::instance().isHardwareAccelerationDisabled();
#ifdef Q_OS_WIN
        QString ver=QSysInfo::productVersion();
        bool oldWin = ver.startsWith("6.0")||ver.startsWith("6.1")||ver.startsWith("5.");
        if(oldWin) {
            hw=false;
            // Windows 7特殊配置：强制禁用可能导致崩溃的功能
            settings->setAttribute(QWebEngineSettings::PluginsEnabled,false);
            settings->setAttribute(QWebEngineSettings::JavascriptCanOpenWindows,false);
            settings->setAttribute(QWebEngineSettings::PlaybackRequiresUserGesture,true);
            Logger::instance().appEvent("检测到Windows 7系统，启用兼容模式", L_INFO);
        }
#endif
        settings->setAttribute(QWebEngineSettings::WebGLEnabled,hw);
        settings->setAttribute(QWebEngineSettings::Accelerated2dCanvasEnabled,hw);
        settings->setAttribute(QWebEngineSettings::ScreenCaptureEnabled,false);
        settings->setAttribute(QWebEngineSettings::WebRTCPublicInterfacesOnly,true);
        
        // Windows 7额外兼容性设置
#ifdef Q_OS_WIN
        if(oldWin) {
            // 禁用GPU进程以避免崩溃
            qputenv("QTWEBENGINE_DISABLE_GPU", "1");
            qputenv("QTWEBENGINE_DISABLE_SANDBOX", "1");
            // 强制软件渲染并禁用GPU合成（解决黑屏问题）
            qputenv("QTWEBENGINE_CHROMIUM_FLAGS", 
                "--disable-gpu "
                "--disable-gpu-compositing "
                "--disable-gpu-sandbox "
                "--disable-software-rasterizer "
                "--num-raster-threads=1 "
                "--enable-viewport "
                "--main-frame-resizes-are-orientation-changes "
                "--disable-composited-antialiasing "
                "--disable-accelerated-2d-canvas "
                "--disable-accelerated-jpeg-decoding "
                "--disable-accelerated-mjpeg-decode "
                "--disable-accelerated-video-decode "
                "--in-process-gpu "
                "--single-process");
            // 强制使用软件OpenGL渲染
            qputenv("QT_OPENGL", "software");
            qputenv("QSG_RHI_PREFER_SOFTWARE_RENDERER", "1");
            Logger::instance().appEvent("应用Windows 7 WebEngine黑屏修复补丁", L_INFO);
        }
#endif

        load(QUrl(ConfigManager::instance().getUrl()));
        Logger::instance().appEvent("程序启动");

        exitHotkeyF10 = new QHotkey(QKeySequence("F10"),true,this);
        exitHotkeyBackslash = new QHotkey(QKeySequence("\\"),true,this);
        connect(exitHotkeyF10,&QHotkey::activated,this,&ShellBrowser::handleExitHotkey);
        connect(exitHotkeyBackslash,&QHotkey::activated,this,&ShellBrowser::handleExitHotkey);

        setWindowFlags(Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
        setWindowState(Qt::WindowFullScreen); showFullScreen();

        maintenanceTimer=new QTimer(this);
        connect(maintenanceTimer,&QTimer::timeout,this,[this](){
            if(needFocusCheck && !isActiveWindow()){ raise(); activateWindow(); }
            if(needFullscreenCheck && windowState()!=Qt::WindowFullScreen){ setWindowState(Qt::WindowFullScreen); showFullScreen(); }
        });
        maintenanceTimer->start(1500);

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
    // Windows 7 WebEngine兼容性：在QApplication创建前设置环境变量
#ifdef Q_OS_WIN
    QString ver = QSysInfo::productVersion();
    bool oldWin = ver.startsWith("6.0")||ver.startsWith("6.1")||ver.startsWith("5.");
    if(oldWin) {
        // 设置WebEngine环境变量（必须在QApplication之前）
        qputenv("QTWEBENGINE_DISABLE_GPU", "1");
        qputenv("QTWEBENGINE_DISABLE_SANDBOX", "1");
        qputenv("QT_OPENGL", "software");
        qputenv("QSG_RHI_PREFER_SOFTWARE_RENDERER", "1");
        qputenv("QTWEBENGINE_CHROMIUM_FLAGS", 
            "--disable-gpu "
            "--disable-gpu-compositing "
            "--disable-gpu-sandbox "
            "--single-process "
            "--in-process-gpu "
            "--disable-dev-shm-usage "
            "--no-sandbox");
    }
#endif

    QApplication app(argc,argv);
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
