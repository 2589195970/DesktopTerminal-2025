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

#ifdef Q_OS_WIN
#include <windows.h>
#include <iostream>
#include <io.h>
#include <fcntl.h>
#endif

// 配置管理类
class ConfigManager {
public:
    static ConfigManager& instance() {
        static ConfigManager configManager;
        return configManager;
    }
    
    bool loadConfig(const QString &configPath = "resources/config.json") {
        // 尝试多种可能的路径
        QStringList possiblePaths;
        
        // 获取可执行文件路径
        QString exePath = QCoreApplication::applicationDirPath();
        
        // 1. 优先级最高：可执行文件同目录下的 config.json
        possiblePaths << exePath + "/config.json";
        
        // 2. 用户配置目录（Windows: %APPDATA%/zdf-exam-desktop, Linux: ~/.config/zdf-exam-desktop, macOS: ~/Library/Application Support/zdf-exam-desktop）
        QString userConfigPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
        if (!userConfigPath.isEmpty()) {
            possiblePaths << userConfigPath + "/config.json";
        }
        
        // 3. 系统配置目录（Linux/Unix）
        #ifdef Q_OS_UNIX
        possiblePaths << "/etc/zdf-exam-desktop/config.json";
        #endif
        
        // 4. 内置资源目录（向后兼容）
        possiblePaths << exePath + "/" + configPath;
        possiblePaths << exePath + "/../" + configPath;
        
        // 5. 直接使用提供的路径
        possiblePaths << configPath;
        
        // 6. 绝对路径尝试
        if (QDir::isAbsolutePath(configPath)) {
            possiblePaths << configPath;
        }
        
        // 输出所有尝试的路径
        qDebug("尝试加载配置文件，按优先级从高到低:");
        foreach (const QString &path, possiblePaths) {
            qDebug(" - %s %s", qPrintable(path), QFile::exists(path) ? "(存在)" : "(不存在)");
        }
        
        // 尝试打开每个可能的路径
        foreach (const QString &path, possiblePaths) {
            QFile file(path);
            if (!file.exists()) {
                continue;
            }
            
            if (!file.open(QIODevice::ReadOnly)) {
                qDebug("文件存在但无法打开: %s, 错误: %s", qPrintable(path), qPrintable(file.errorString()));
                continue;
            }
            
            QByteArray data = file.readAll();
            file.close();
            
            QJsonParseError jsonError;
            QJsonDocument doc = QJsonDocument::fromJson(data, &jsonError);
            
            if (doc.isNull() || !doc.isObject()) {
                qDebug("JSON解析错误: %s, 位置: %d", qPrintable(jsonError.errorString()), jsonError.offset);
                continue;
            }
            
            config = doc.object();
            
            // 检查必要的配置项是否存在
            if (!validateConfig()) {
                qDebug("配置文件格式不正确，缺少必要的配置项");
                continue;  // 继续尝试下一个文件而不是直接返回
            }
            
            qDebug("成功加载配置文件: %s", qPrintable(path));
            qDebug("配置内容: URL=%s, 应用名称=%s, 退出密码=%s", 
                   qPrintable(getUrl()), 
                   qPrintable(getAppName()),
                   qPrintable(QString("*").repeated(getExitPassword().length())));  // 密码用星号显示
            
            // 记录实际使用的配置文件路径
            actualConfigPath = path;
            return true;
        }
        
        qDebug("无法加载配置文件，所有路径尝试均失败");
        return false;
    }
    
    // 检查配置文件是否包含所有必要的字段
    bool validateConfig() const {
        // 检查必要的配置项
        QStringList requiredKeys = {"url", "exitPassword", "appName"};
        foreach (const QString &key, requiredKeys) {
            if (!config.contains(key) || config.value(key).toString().isEmpty()) {
                qDebug("配置文件缺少必要的字段: %s", qPrintable(key));
                return false;
            }
        }
        return true;
    }
    
    QString getUrl() const {
        return config.value("url").toString("http://stu.sdzdf.com/");
    }
    
    QString getExitPassword() const {
        return config.value("exitPassword").toString("123456");
    }
    
    QString getAppName() const {
        return config.value("appName").toString("zdf-exam-desktop");
    }
    
    bool isHardwareAccelerationDisabled() const {
        return config.value("disableHardwareAcceleration").toBool(false);
    }
    
    QString getActualConfigPath() const {
        return actualConfigPath;
    }
    
    // 创建默认配置文件
    bool createDefaultConfig(const QString &path) {
        QJsonObject defaultConfig;
        defaultConfig["url"] = "http://stu.sdzdf.com/";
        defaultConfig["exitPassword"] = "sdzdf@2025";
        defaultConfig["appName"] = "智多分机考桌面端";
        defaultConfig["iconPath"] = "logo.svg";
        defaultConfig["appVersion"] = "1.0.0";
        defaultConfig["disableHardwareAcceleration"] = false;
        
        QJsonDocument doc(defaultConfig);
        QFile file(path);
        
        // 确保目录存在
        QFileInfo fileInfo(path);
        QDir dir = fileInfo.dir();
        if (!dir.exists()) {
            if (!dir.mkpath(".")) {
                qDebug("无法创建目录: %s", qPrintable(dir.path()));
                return false;
            }
        }
        
        if (!file.open(QIODevice::WriteOnly)) {
            qDebug("无法创建配置文件: %s, 错误: %s", qPrintable(path), qPrintable(file.errorString()));
            return false;
        }
        
        file.write(doc.toJson());
        file.close();
        
        qDebug("已创建默认配置文件: %s", qPrintable(path));
        return true;
    }
    
    QJsonObject config;
    
private:
    ConfigManager() {
        loadConfig();
    }
    
    QString actualConfigPath;
};

// 前向声明
class QTimer;

// 日志级别枚举
enum LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR
};

// 日志条目结构
struct LogEntry {
    QDateTime timestamp;
    QString category;
    QString message;
    QString filename;
};

// 简化的日志管理类，避免使用QObject和信号槽
class Logger {
private:
    // 私有构造函数，防止外部实例化
    Logger() : m_logLevel(INFO) {
        // 初始化时设置应用程序默认编码
        QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));
        
        // 启动定时刷新定时器
        m_flushTimer = new QTimer();
        QObject::connect(m_flushTimer, &QTimer::timeout, [this]() {
            this->flushAllLogBuffers();
        });
        m_flushTimer->start(5000); // 每5秒刷新一次日志
    }
    
    // 禁止拷贝和赋值
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    
    ~Logger() {
        flushAllLogBuffers();
        if (m_flushTimer) {
            m_flushTimer->stop();
            delete m_flushTimer;
        }
    }
    
    // 成员变量
    static const int LOG_BUFFER_SIZE = 10; // 缓冲区大小
    QMap<QString, QList<LogEntry>> m_logBuffer; // 按文件名分组的日志缓冲区
    LogLevel m_logLevel; // 日志级别
    QTimer* m_flushTimer; // 定时刷新定时器
    
public:
    // 单例访问点
    static Logger& instance() {
        static Logger logger;
        return logger;
    }
    
    // 设置日志级别
    void setLogLevel(LogLevel level) {
        m_logLevel = level;
    }
    
    LogLevel getLogLevel() const {
        return m_logLevel;
    }
    
    // 确保日志目录存在
    bool ensureLogDirectoryExists() {
        QString logDir = QCoreApplication::applicationDirPath() + "/log";
        QDir dir(logDir);
        if (!dir.exists()) {
            if (!dir.mkpath(".")) {
                qDebug("无法创建日志目录: %s", qPrintable(logDir));
                return false;
            }
            qDebug("已创建日志目录: %s", qPrintable(logDir));
        }
        return true;
    }
    
    // 记录应用程序事件到指定日志文件
    void logEvent(const QString &category, const QString &message, const QString &filename = "app.log", LogLevel level = INFO) {
        // 检查日志级别
        if (level < m_logLevel) {
            return;
        }
        
        // 添加到日志缓冲区
        LogEntry entry;
        entry.timestamp = QDateTime::currentDateTime();
        entry.category = category;
        entry.message = message;
        entry.filename = filename;
        
        m_logBuffer[filename].append(entry);
        
        // 如果缓冲区达到一定大小或者重要日志，立即刷新
        if (m_logBuffer[filename].size() >= LOG_BUFFER_SIZE || level >= WARNING) {
            flushLogBuffer(filename);
        }
    }
    
    // 刷新特定文件的日志缓冲区
    void flushLogBuffer(const QString &filename) {
        if (m_logBuffer[filename].isEmpty()) {
            return;
        }
        
        // 确保日志目录存在
        if (!ensureLogDirectoryExists()) {
            return;
        }
        
        QString logDir = QCoreApplication::applicationDirPath() + "/log";
        QString logFilePath = logDir + "/" + filename;
        
        QFile file(logFilePath);
        if (file.open(QIODevice::Append | QIODevice::Text)) {
            QTextStream out(&file);
            out.setCodec("UTF-8"); // 统一使用UTF-8编码
            
            foreach (const LogEntry &entry, m_logBuffer[filename]) {
                out << entry.timestamp.toString("yyyy-MM-dd hh:mm:ss")
                    << " | " << entry.category << " | " << entry.message << "\n";
            }
            
            file.close();
            m_logBuffer[filename].clear();
        } else {
            qDebug("无法写入日志文件: %s, 错误: %s", qPrintable(logFilePath), qPrintable(file.errorString()));
        }
    }
    
    // 刷新所有日志缓冲区
    void flushAllLogBuffers() {
        QStringList filenames = m_logBuffer.keys();
        foreach (const QString &filename, filenames) {
            flushLogBuffer(filename);
        }
    }
    
    // 应用程序事件
    void appEvent(const QString &event, LogLevel level = INFO) {
        logEvent("应用程序", event, "app.log", level);
    }
    
    // 配置加载事件
    void configEvent(const QString &event, LogLevel level = INFO) {
        logEvent("配置文件", event, "config.log", level);
    }
    
    // 记录启动信息，包括配置文件位置
    void logStartup(const QString &configPath) {
        QString message = QString("程序启动成功，使用配置文件: %1").arg(configPath);
        logEvent("启动", message, "startup.log", INFO);
        qDebug("%s", qPrintable(message));
    }
    
    // 热键事件
    void hotkeyEvent(const QString &result) {
        logEvent("热键退出尝试", result, "exit.log", INFO);
    }
    
    // 显示消息框（确保UI文本也使用正确编码）
    void showMessage(QWidget *parent, const QString &title, const QString &message) {
        QMessageBox::warning(parent, title, message);
    }
    
    // 获取文本输入（确保输入框也使用正确编码）
    bool getPassword(QWidget *parent, const QString &title, const QString &label, QString &password) {
        bool ok;
        password = QInputDialog::getText(parent, title, label, QLineEdit::Password, "", &ok);
        return ok;
    }
    
    // 程序退出时调用
    void shutdown() {
        flushAllLogBuffers();
        if (m_flushTimer) {
            m_flushTimer->stop();
        }
    }
};

class ShellBrowser : public QWebEngineView {
private:
    QHotkey* exitHotkeyF10;
    QHotkey* exitHotkeyBackslash;
    QTimer* maintenanceTimer; // 合并后的维护定时器
    bool needFocusCheck;
    bool needFullscreenCheck;
    
public:
    ShellBrowser() {
        // 从配置文件读取应用名称和URL
        setWindowTitle(ConfigManager::instance().getAppName());
        setMinimumSize(1280, 800);
        
        // WebEngine性能优化 - 兼容性优先
        QWebEngineSettings *settings = QWebEngineSettings::globalSettings();
        
        // 基础设置 - 确保基本功能正常
        settings->setAttribute(QWebEngineSettings::JavascriptEnabled, true);
        settings->setAttribute(QWebEngineSettings::AutoLoadImages, true);
        settings->setAttribute(QWebEngineSettings::PluginsEnabled, true); // 保留插件支持
        settings->setAttribute(QWebEngineSettings::LocalStorageEnabled, true);
        
        // 允许页面功能 - 确保教育网站功能完整性
        settings->setAttribute(QWebEngineSettings::PlaybackRequiresUserGesture, false); // 允许自动播放视频
        settings->setAttribute(QWebEngineSettings::JavascriptCanOpenWindows, true); // 允许打开新窗口
        
        // 条件启用硬件加速 - 带有兼容性检查
        bool enableHardwareAcceleration = !ConfigManager::instance().isHardwareAccelerationDisabled();
        
        // 检查平台兼容性
        #if defined(Q_OS_WIN)
            // Windows 平台特殊处理
            DWORD dwVersion = GetVersion();
            DWORD dwMajorVersion = (DWORD)(LOBYTE(LOWORD(dwVersion)));
            
            // Windows 7或更低版本(Windows 7 = 6.1)可能存在WebGL兼容性问题
            if (dwMajorVersion < 6 || (dwMajorVersion == 6 && HIBYTE(LOWORD(dwVersion)) <= 1)) {
                Logger::instance().appEvent("检测到Windows 7或更低版本，禁用硬件加速以提高兼容性", INFO);
                enableHardwareAcceleration = false;
            }
        #endif
        
        if (enableHardwareAcceleration) {
            // 谨慎启用硬件加速功能
            settings->setAttribute(QWebEngineSettings::WebGLEnabled, true);
            settings->setAttribute(QWebEngineSettings::Accelerated2dCanvasEnabled, true);
            
            Logger::instance().appEvent("已启用WebGL和2D Canvas加速", INFO);
        } else {
            // 禁用硬件加速
            settings->setAttribute(QWebEngineSettings::WebGLEnabled, false);
            settings->setAttribute(QWebEngineSettings::Accelerated2dCanvasEnabled, false);
            Logger::instance().appEvent("已禁用硬件加速功能", INFO);
        }
        
        // 禁用可能影响安全的功能
        settings->setAttribute(QWebEngineSettings::ScreenCaptureEnabled, false);
        settings->setAttribute(QWebEngineSettings::WebRTCPublicInterfacesOnly, true);
        
        // 加载网页
        load(QUrl(ConfigManager::instance().getUrl()));
        Logger::instance().appEvent("程序启动");
        
        // 注册全局热键
        exitHotkeyF10 = new QHotkey(QKeySequence("F10"), true, this);
        exitHotkeyBackslash = new QHotkey(QKeySequence("\\"), true, this);
        
        connect(exitHotkeyF10, &QHotkey::activated, this, &ShellBrowser::handleExitHotkey);
        connect(exitHotkeyBackslash, &QHotkey::activated, this, &ShellBrowser::handleExitHotkey);
        
        // 强制全屏、无边框、置顶
        setWindowFlags(Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
        setWindowState(Qt::WindowFullScreen);
        showFullScreen();
        
        // 合并定时器，一个定时器处理多个任务
        needFocusCheck = true;
        needFullscreenCheck = true;
        
        maintenanceTimer = new QTimer(this);
        connect(maintenanceTimer, &QTimer::timeout, this, [this]() {
            // 检查焦点
            if (needFocusCheck && !this->isActiveWindow()) {
                this->raise();
                this->activateWindow();
                Logger::instance().appEvent("应用程序重新获取焦点", DEBUG);
            }
            
            // 检查全屏状态
            if (needFullscreenCheck && this->windowState() != Qt::WindowFullScreen) {
                Logger::instance().appEvent("检测到非全屏状态，正在恢复全屏", INFO);
                this->setWindowState(Qt::WindowFullScreen);
                this->showFullScreen();
            }
        });
        maintenanceTimer->start(1500); // 每1.5秒执行一次检查，减少CPU使用
        
        // 禁用右键菜单
        setContextMenuPolicy(Qt::NoContextMenu);
        
        // 注册刷新快捷键
        QShortcut* refreshShortcut = new QShortcut(QKeySequence("Ctrl+R"), this);
        connect(refreshShortcut, &QShortcut::activated, this, [this]() {
            this->reload();
            Logger::instance().appEvent("用户使用Ctrl+R刷新页面", INFO);
        });
    }
    
    // 允许启用/禁用焦点检查
    void setFocusCheckEnabled(bool enabled) {
        needFocusCheck = enabled;
    }
    
    // 允许启用/禁用全屏检查
    void setFullscreenCheckEnabled(bool enabled) {
        needFullscreenCheck = enabled;
    }
    
protected:
    // 阻止右键菜单
    void contextMenuEvent(QContextMenuEvent *event) override {
        event->ignore();
    }
    
    // 拦截更多系统快捷键
    bool event(QEvent *event) override {
        if (event->type() == QEvent::ShortcutOverride) {
            QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
            
            // 允许Ctrl+R通过
            if (keyEvent->key() == Qt::Key_R && keyEvent->modifiers() == Qt::ControlModifier) {
                return QWebEngineView::event(event);
            }
            
            // 记录检测到的组合键
            if (keyEvent->modifiers() != Qt::NoModifier) {
                QString keySequence = QKeySequence(keyEvent->key() | keyEvent->modifiers()).toString();
                Logger::instance().appEvent(QString("检测到快捷键: %1").arg(keySequence), INFO);
            }
            
            // 拦截所有其他组合键
            if (keyEvent->modifiers() != Qt::NoModifier) {
                event->accept();
                return true;
            }
        }
        return QWebEngineView::event(event);
    }
    
    void handleExitHotkey() {
        // 暂停焦点定时器，避免干扰密码输入
        setFocusCheckEnabled(false);
        
        QString pwd;
        bool ok = Logger::instance().getPassword(this, "安全退出", "请输入退出密码：", pwd);
        
        // 从配置文件获取退出密码
        QString exitPassword = ConfigManager::instance().getExitPassword();
        
        if (ok && pwd == exitPassword) {
            Logger::instance().hotkeyEvent("密码正确，退出");
            Logger::instance().appEvent("程序退出");
            Logger::instance().shutdown(); // 确保刷新所有日志
            QApplication::quit();
        } else if (ok) {
            Logger::instance().hotkeyEvent("密码错误");
            Logger::instance().showMessage(this, "错误", "密码错误");
            // 密码错误，重新启动定时器
            setFocusCheckEnabled(true);
        } else {
            Logger::instance().hotkeyEvent("取消输入");
            // 取消输入，重新启动定时器
            setFocusCheckEnabled(true);
        }
    }
    
    void closeEvent(QCloseEvent *event) override {
        // 只有通过热键正确密码才能关闭
        event->ignore();
    }
    
    void focusOutEvent(QFocusEvent *event) override {
        // 优化：不是每次失去焦点就立即抢回，而是由定时器统一处理
        QWebEngineView::focusOutEvent(event);
    }
    
    void keyPressEvent(QKeyEvent *event) override {
        // 记录所有按键
        QString keyName = QKeySequence(event->key() | event->modifiers()).toString();
        Logger::instance().appEvent(QString("按键事件: %1").arg(keyName), INFO);
        
        // 处理Ctrl+R刷新页面
        if (event->key() == Qt::Key_R && event->modifiers() == Qt::ControlModifier) {
            reload();
            Logger::instance().appEvent("执行页面刷新操作", INFO);
            event->accept();
            return;
        }
        
        // 拦截 Command+Option+Escape (Force Quit)
        if (event->key() == Qt::Key_Escape && 
            (event->modifiers() & (Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier))) {
            event->ignore();
            return;
        }
        QWebEngineView::keyPressEvent(event);
    }
};

// 全局事件过滤器，捕获并拦截系统级快捷键
class GlobalEventFilter : public QObject {
protected:
    bool eventFilter(QObject *obj, QEvent *event) override {
        if (event->type() == QEvent::KeyPress) {
            QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
            
            // 记录所有按键
            if (keyEvent->modifiers() != Qt::NoModifier || keyEvent->key() >= Qt::Key_F1) {
                QString keyName = QKeySequence(keyEvent->key() | keyEvent->modifiers()).toString();
                Logger::instance().appEvent(QString("全局快捷键: %1").arg(keyName), INFO);
            }
            
            // 快速路径：允许Ctrl+R刷新页面通过
            if (keyEvent->key() == Qt::Key_R && 
                keyEvent->modifiers() == Qt::ControlModifier) {
                return false; // 允许事件传递
            }
            
            // 拦截所有组合键
            if (keyEvent->modifiers() != Qt::NoModifier) {
                event->accept();
                return true;
            }
            
            // 拦截 Alt+Tab/Option+Tab (App Switcher)
            if (keyEvent->key() == Qt::Key_Tab && 
                ((keyEvent->modifiers() & Qt::AltModifier) || (keyEvent->modifiers() & Qt::MetaModifier))) {
                return true;
            }
            
            // 拦截 Ctrl+Alt+Del (任务管理器)
            if (keyEvent->key() == Qt::Key_Delete && 
                (keyEvent->modifiers() & Qt::ControlModifier) && 
                (keyEvent->modifiers() & Qt::AltModifier)) {
                return true;
            }
            
            // 拦截 Command+Option+Escape (Force Quit)
            if (keyEvent->key() == Qt::Key_Escape &&
                (keyEvent->modifiers() & (Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier))) {
                return true;
            }
            
            // 拦截 Alt+F4/Command+Q (关闭窗口/退出应用)
            if ((keyEvent->key() == Qt::Key_F4 && (keyEvent->modifiers() & Qt::AltModifier)) || 
                (keyEvent->key() == Qt::Key_Q && (keyEvent->modifiers() & Qt::MetaModifier))) {
                return true;
            }
            
            // 拦截 Windows键/Command键 (开始菜单/Dock)
            if (keyEvent->key() == Qt::Key_Meta) {
                return true;
            }
            
            // 拦截任务视图快捷键 (Win+Tab/Mission Control)
            if ((keyEvent->key() == Qt::Key_Tab && (keyEvent->modifiers() & Qt::MetaModifier)) ||
                // Mission Control (Ctrl+Up) on Mac
                (keyEvent->key() == Qt::Key_Up && (keyEvent->modifiers() & Qt::ControlModifier))) {
                return true;
            }
            
            // 拦截 Cmd+Space (Spotlight on Mac)
            if (keyEvent->key() == Qt::Key_Space && (keyEvent->modifiers() & Qt::MetaModifier)) {
                return true;
            }
            
            // 拦截 Cmd+H (隐藏窗口 on Mac)
            if (keyEvent->key() == Qt::Key_H && (keyEvent->modifiers() & Qt::MetaModifier)) {
                return true;
            }
            
            // 拦截 Cmd+M (最小化窗口 on Mac)
            if (keyEvent->key() == Qt::Key_M && (keyEvent->modifiers() & Qt::MetaModifier)) {
                return true;
            }
        }
        
        // 拦截窗口状态变化事件
        if (event->type() == QEvent::WindowStateChange) {
            QWindowStateChangeEvent *stateEvent = static_cast<QWindowStateChangeEvent*>(event);
            if (!(stateEvent->oldState() & Qt::WindowFullScreen)) {
                // 如果窗口要从全屏状态改变，阻止该操作
                QWindow *window = qobject_cast<QWindow*>(obj);
                if (window) {
                    window->setWindowState(Qt::WindowFullScreen);
                    Logger::instance().appEvent("拦截窗口状态变化，强制保持全屏", INFO);
                    return true;
                }
            }
        }
        
        return QObject::eventFilter(obj, event);
    }
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    // 设置全局应用程序编码
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));
    
#ifdef Q_OS_WIN
    // Windows 平台特殊处理
    #if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        // Qt5 需要设置更多编码
        QTextCodec *codec = QTextCodec::codecForName("UTF-8");
        QTextCodec::setCodecForLocale(codec);
    #endif
    // 设置 Windows 控制台编码
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
    
    // 设置应用程序属性
    app.setApplicationName("DesktopTerminal");
    app.setOrganizationName("智多分");
    
    // 设置全屏窗口应用程序属性
    app.setQuitOnLastWindowClosed(false); // 防止最后一个窗口关闭时应用退出
    
#ifdef Q_OS_MAC
    // Mac 平台特殊处理
    app.setAttribute(Qt::AA_PluginApplication, true); // 隐藏 Dock 图标
#endif
    
    // 设置日志级别 - 正式环境可以设置为INFO或WARNING减少日志量
#ifdef QT_DEBUG
    Logger::instance().setLogLevel(DEBUG);
#else
    Logger::instance().setLogLevel(INFO);
#endif
    
    // 确保日志目录存在
    Logger::instance().ensureLogDirectoryExists();
    Logger::instance().appEvent("应用程序初始化...");
    
    // 输出当前工作目录，帮助调试
    qDebug("当前工作目录: %s", qPrintable(QDir::currentPath()));
    qDebug("应用程序目录: %s", qPrintable(QCoreApplication::applicationDirPath()));
    
    // 记录系统环境信息
    QString systemInfo = QString("系统信息: %1, Qt版本: %2")
                        .arg(QSysInfo::prettyProductName())
                        .arg(qVersion());
    Logger::instance().appEvent(systemInfo);
    
    // 加载配置文件
    ConfigManager &configManager = ConfigManager::instance();
    if (!configManager.loadConfig()) {
        qDebug("配置文件加载失败，尝试创建默认配置文件");
        Logger::instance().configEvent("配置文件加载失败，尝试创建默认配置文件");
        
        // 优先在应用程序目录下创建 config.json（方便修改）
        QString defaultConfigPath = QCoreApplication::applicationDirPath() + "/config.json";
        if (configManager.createDefaultConfig(defaultConfigPath) && configManager.loadConfig(defaultConfigPath)) {
            qDebug("已创建并加载默认配置文件: %s", qPrintable(defaultConfigPath));
            Logger::instance().configEvent(QString("已创建并加载默认配置文件: %1").arg(defaultConfigPath));
            QMessageBox::information(nullptr, "提示", 
                QString("已创建默认配置文件:\n%1\n\n"
                        "您可以编辑此文件来修改应用配置:\n"
                        "- url: 指向的网址\n"
                        "- exitPassword: 退出密码\n"
                        "- appName: 应用名称").arg(defaultConfigPath));
        } else {
            // 如果无法在应用程序目录创建，尝试在 resources 子目录
            defaultConfigPath = QCoreApplication::applicationDirPath() + "/resources/config.json";
            if (configManager.createDefaultConfig(defaultConfigPath) && configManager.loadConfig(defaultConfigPath)) {
                qDebug("已创建并加载默认配置文件: %s", qPrintable(defaultConfigPath));
                Logger::instance().configEvent(QString("已创建并加载默认配置文件: %1").arg(defaultConfigPath));
            } else {
                qDebug("无法创建或加载默认配置文件，程序将退出");
                Logger::instance().configEvent("无法创建或加载默认配置文件，程序将退出");
                QMessageBox::critical(nullptr, "错误", 
                    "无法加载配置文件，程序将退出。\n"
                    "请确保程序有权限在应用程序目录创建文件。");
                return 1;
            }
        }
    }
    
    // 记录成功加载的配置文件路径
    Logger::instance().logStartup(configManager.getActualConfigPath());
    
    // 安装全局事件过滤器
    GlobalEventFilter *filter = new GlobalEventFilter();
    app.installEventFilter(filter);
    
    ShellBrowser browser;
    browser.showFullScreen();
    
    // 程序退出前确保所有日志都被写入
    QObject::connect(&app, &QApplication::aboutToQuit, []() {
        Logger::instance().shutdown();
    });
    
    return app.exec();
} 