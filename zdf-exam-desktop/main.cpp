#include <QApplication>
#include <QWebEngineView>
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
    
private:
    ConfigManager() {
        loadConfig();
    }
    
    QJsonObject config;
    QString actualConfigPath;
};

// 统一日志管理类
class Logger {
public:
    static Logger& instance() {
        static Logger logger;
        return logger;
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
    void logEvent(const QString &category, const QString &message, const QString &filename = "app.log") {
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
            out << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")
                << " | " << category << " | " << message << "\n";
            file.close();
        } else {
            qDebug("无法写入日志文件: %s, 错误: %s", qPrintable(logFilePath), qPrintable(file.errorString()));
        }
    }
    
    // 应用程序事件
    void appEvent(const QString &event) {
        logEvent("应用程序", event);
    }
    
    // 配置加载事件
    void configEvent(const QString &event) {
        logEvent("配置文件", event, "config.log");
    }
    
    // 记录启动信息，包括配置文件位置
    void logStartup(const QString &configPath) {
        QString message = QString("程序启动成功，使用配置文件: %1").arg(configPath);
        logEvent("启动", message, "startup.log");
        qDebug("%s", qPrintable(message));
    }
    
    // 热键事件
    void hotkeyEvent(const QString &result) {
        logEvent("热键退出尝试", result, "exit.log");
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
    
private:
    Logger() {
        // 初始化时设置应用程序默认编码
        QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));
    }
};

class ShellBrowser : public QWebEngineView {
    Q_OBJECT
private:
    QHotkey* exitHotkeyF10;
    QHotkey* exitHotkeyBackslash;
    QTimer* focusTimer;
    
public:
    ShellBrowser() {
        // 从配置文件读取应用名称和URL
        setWindowTitle(ConfigManager::instance().getAppName());
        setMinimumSize(1280, 800);
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
        
        // 定时器抢回焦点
        focusTimer = new QTimer(this);
        connect(focusTimer, &QTimer::timeout, this, [this]() {
            if (!this->isActiveWindow()) {
                this->raise();
                this->activateWindow();
            }
        });
        focusTimer->start(500);
        
        // 禁用右键菜单
        setContextMenuPolicy(Qt::NoContextMenu);
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
            // 拦截所有组合键
            if (keyEvent->modifiers() != Qt::NoModifier) {
                event->accept();
                return true;
            }
        }
        return QWebEngineView::event(event);
    }
    
    void handleExitHotkey() {
        // 暂停焦点定时器，避免干扰密码输入
        focusTimer->stop();
        
        QString pwd;
        bool ok = Logger::instance().getPassword(this, "安全退出", "请输入退出密码：", pwd);
        
        // 从配置文件获取退出密码
        QString exitPassword = ConfigManager::instance().getExitPassword();
        
        if (ok && pwd == exitPassword) {
            Logger::instance().hotkeyEvent("密码正确，退出");
            Logger::instance().appEvent("程序退出");
            QApplication::quit();
        } else if (ok) {
            Logger::instance().hotkeyEvent("密码错误");
            Logger::instance().showMessage(this, "错误", "密码错误");
            // 密码错误，重新启动定时器
            focusTimer->start(500);
        } else {
            Logger::instance().hotkeyEvent("取消输入");
            // 取消输入，重新启动定时器
            focusTimer->start(500);
        }
    }
    
    void closeEvent(QCloseEvent *event) override {
        // 只有通过热键正确密码才能关闭
        event->ignore();
    }
    
    void focusOutEvent(QFocusEvent *event) override {
        this->raise();
        this->activateWindow();
        QWebEngineView::focusOutEvent(event);
    }
    
    void keyPressEvent(QKeyEvent *event) override {
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
            // 拦截 Command+Option+Escape (Force Quit)
            if (keyEvent->key() == Qt::Key_Escape &&
                (keyEvent->modifiers() & (Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier))) {
                return true;
            }
            // 拦截 Command+Tab (App Switcher)
            if (keyEvent->key() == Qt::Key_Tab && 
                (keyEvent->modifiers() & (Qt::MetaModifier | Qt::AltModifier))) {
                return true;
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
    return app.exec();
}

#include "main.moc" 