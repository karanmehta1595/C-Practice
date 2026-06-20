#include <QApplication>
#include <QMainWindow>
#include <QWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMenu>
#include <QComboBox>
#include <QCompleter>
#include <QSettings>
#include <QLocale>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QSizeGrip>
#include <QMap>
#include <QStack>
#include <cmath>
#include <climits>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>
#include <QIcon>
#include <QRegularExpression>
#include <QClipboard>
#include <QToolTip>
#include <QScrollArea>
#include <QEvent>
#include <QApplication>
#include <QJsonArray>
#include <QFile>
#include <functional>

#include "googleauthmanager.h"
#include "googledrivebackup.h"
#include "calculatorengine.h"
#include "historymanager.h"
#include "currencyservice.h"
#include "themeservice.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#ifndef M_E
#define M_E  2.71828182845904523536
#endif

// ─────────────────────────────────────────────
//  VIEW MODES
// ─────────────────────────────────────────────
enum ViewMode {
    MODE_STANDARD   = 0,
    MODE_SCIENTIFIC = 1,
    MODE_CURRENCY   = 2,
    MODE_GEOMETRY   = 3,
    MODE_UNIT       = 4,
    MODE_EMI        = 5,
    MODE_GST        = 6,
};

// ─────────────────────────────────────────────
//  HELPERS
// ─────────────────────────────────────────────
/*static QString formatResult(double v) {
    if (v == std::floor(v) && std::abs(v) < 1e15)
        return QString::number((long long)v);
    return QString::number(v, 'g', 10);
}*/

// ─────────────────────────────────────────────
//  MAIN CLASS
// ─────────────────────────────────────────────
class OS1Calculator : public QMainWindow {
public:
    // ── Widgets ──────────────────────────────
    QLabel      *expressionLabel  = nullptr;
    QLineEdit   *display          = nullptr;

    // New Toolbar & Menus
    QWidget     *toolbarPanel     = nullptr;
    QPushButton *toolsBtn         = nullptr;
    QPushButton *findBtn          = nullptr;
    QPushButton *aboutBtn         = nullptr;
    QPushButton *historyBtn       = nullptr;

    QWidget     *toolsPopup       = nullptr;
    QWidget     *themesSubmenu    = nullptr;
    QWidget     *aboutPopup       = nullptr;

    QLineEdit   *quickSearch      = nullptr;
    QWidget     *centralWidget    = nullptr;
    QWidget     *topBarPanel      = nullptr;
    QLabel      *titleLabel       = nullptr;
    QListWidget *historyList      = nullptr;
    QWidget     *historyWidget    = nullptr;
    QWidget     *gridContainer    = nullptr;
    QGridLayout *buttonGrid       = nullptr;
    QPushButton *btnMin           = nullptr;
    QPushButton *btnMax           = nullptr;
    QPushButton *btnClose         = nullptr;
    QTimer      *themeWatcher     = nullptr;

    // Currency widgets
    QComboBox  *fromCurrencyCombo   = nullptr;
    QComboBox  *toCurrencyCombo     = nullptr;
    QLineEdit  *currencyInput       = nullptr;
    QLabel     *currencyResultLabel = nullptr;

    // History search
    QLineEdit *historySearchBox = nullptr;
    bool       history_search_visible = false;

    // ── State ────────────────────────────────
    // ── State ────────────────────────────────
    // ── State ────────────────────────────────

    double  current_val        = 0.0;
    double  last_val           = 0.0;
    QString last_op            = "";
    bool    start_new_num      = true;
    bool    expressionMode     = false;
    bool    is2ndMode          = false;
    bool    isDegMode          = true;
    int     current_view_mode  = MODE_STANDARD;
    bool    is_history_visible = false;
    bool    isMaximizedState   = false;
    QString searchTarget;

    // History entries managed by HistoryManager

    // ── Google Sign-In / Drive Backup ────────
    GoogleAuthManager   *googleAuth     = nullptr;
    GoogleDriveBackup    *driveBackup    = nullptr;
    QTimer               *autoBackupTimer = nullptr;
    QLabel                *aboutEmailLabel = nullptr; // live-updated when sign-in completes
    QLabel                *aboutPhotoLabel = nullptr;
    QPushButton           *aboutLoginBtn   = nullptr;
    bool                   backupInFlight  = false;

    // ─────────────────────────────────────────
    OS1Calculator() {
        setWindowFlags(Qt::FramelessWindowHint | Qt::Window | Qt::WindowMinMaxButtonsHint);
        setAttribute(Qt::WA_TranslucentBackground);
        setMouseTracking(true);
        setMinimumSize(360, 540);

        // Initialize Services
        HistoryManager::instance()->loadHistory();
        connect(HistoryManager::instance(), &HistoryManager::historyChanged, this, [this](){
            rebuildHistoryList(historySearchBox ? historySearchBox->text().trimmed() : QString());
        });

        CurrencyService::instance()->fetchLiveRates();
        connect(CurrencyService::instance(), &CurrencyService::ratesUpdated, this, [this](){
            if (current_view_mode == MODE_CURRENCY) renderActiveMatrix();
        });

        connect(ThemeService::instance(), &ThemeService::themeChanged, this, &OS1Calculator::Apply1OSStyleSheet);

        buildUI();
        rebuildHistoryList(QString());
        renderActiveMatrix();

        // ── Google Sign-In / Drive Backup setup ──
        googleAuth  = new GoogleAuthManager(this);
        driveBackup = new GoogleDriveBackup(googleAuth, this);

        QString clientJsonPath = QApplication::applicationDirPath() + "/client_secret.json";
        if (!googleAuth->loadClientConfig(clientJsonPath)) {
            qWarning() << "Google Sign-In disabled: could not load" << clientJsonPath;
        }

        connect(googleAuth, &GoogleAuthManager::signedIn, this, [this]() {
            refreshAboutSignInUi();
        });
        connect(googleAuth, &GoogleAuthManager::signInFailed, this, [this](const QString &err) {
            if (aboutPopup) QToolTip::showText(QCursor::pos(), "Sign-in failed: " + err);
        });
        connect(googleAuth, &GoogleAuthManager::signedOut, this, [this]() {
            refreshAboutSignInUi();
        });
        connect(googleAuth, &GoogleAuthManager::profileUpdated, this, [this]() {
            refreshAboutSignInUi();
        });

        setupAutoBackupTimer();

        themeWatcher = new QTimer(this);
        connect(themeWatcher, &QTimer::timeout, this, [this]() {
            if (ThemeService::instance()->getThemeSetting() == 2) Apply1OSStyleSheet();
        });
        themeWatcher->start(1000);
    }

    // ─────────────────────────────────────────
    //  BUILD UI
    // ─────────────────────────────────────────
    void buildUI() {
        centralWidget = new QWidget(this);
        centralWidget->setObjectName("centralWidget");
        auto *rootLayout = new QVBoxLayout(centralWidget);
        rootLayout->setContentsMargins(0, 0, 0, 0);
        rootLayout->setSpacing(0);

        // ── Top bar ──────────────────────────
        topBarPanel = new QWidget(centralWidget);
        topBarPanel->setObjectName("topBarPanel");
        topBarPanel->setFixedHeight(40);
        auto *topBarLayout = new QHBoxLayout(topBarPanel);
        topBarLayout->setContentsMargins(15, 0, 15, 0);
        topBarLayout->setSpacing(8);

        auto makeTrafficBtn = [&](const QString &name, const QString &tip) -> QPushButton* {
            auto *b = new QPushButton("", topBarPanel);
            b->setObjectName(name);
            b->setToolTip(tip);
            b->setFixedSize(14, 14);
            b->installEventFilter(this);
            return b;
        };
        btnClose = makeTrafficBtn("btnClose", "Quit");
        btnMin   = makeTrafficBtn("btnMin",   "Minimise");
        btnMax   = makeTrafficBtn("btnMax",   "Maximise");

        topBarLayout->addWidget(btnClose);
        topBarLayout->addWidget(btnMin);
        topBarLayout->addWidget(btnMax);
        topBarLayout->addSpacing(4);

        titleLabel = new QLabel("Calculator", topBarPanel);
        titleLabel->setObjectName("titleLabel");
        titleLabel->setMinimumWidth(60);
        topBarLayout->addWidget(titleLabel);
        topBarLayout->addStretch();

        connect(btnClose, &QPushButton::clicked, this, [this](){
            qApp->quit();
        });
        connect(btnMin, &QPushButton::clicked, this, &OS1Calculator::showMinimized);
        connect(btnMax, &QPushButton::clicked, this, &OS1Calculator::handleMaximizeToggle);

        // ── Inner content ────────────────────
        auto *innerContent = new QWidget(centralWidget);
        innerContent->setObjectName("innerContent");
        auto *innerLayout  = new QVBoxLayout(innerContent);
        innerLayout->setContentsMargins(14, 10, 14, 12);
        innerLayout->setSpacing(6);

        buildToolbar(innerLayout, innerContent);
        buildDisplayCard(innerLayout, innerContent);
        buildSearchBar(innerLayout, innerContent);
        buildHistoryDrawer(innerLayout, innerContent);

        gridContainer = new QWidget(innerContent);
        gridContainer->setObjectName("gridContainer");
        buttonGrid = new QGridLayout(gridContainer);
        buttonGrid->setAlignment(Qt::AlignTop);
        buttonGrid->setContentsMargins(0, 0, 0, 0);
        buttonGrid->setSpacing(3);
        innerLayout->addWidget(gridContainer, 1);

        auto *gripBox = new QHBoxLayout();
        gripBox->addStretch();
        gripBox->addWidget(new QSizeGrip(this));
        innerLayout->addLayout(gripBox);

        rootLayout->addWidget(topBarPanel);
        rootLayout->addWidget(innerContent);
        setCentralWidget(centralWidget);
    }

        void buildToolbar(QVBoxLayout *innerLayout, QWidget *innerContent) {
            auto *toolbarFrame = new QFrame(innerContent);
            toolbarFrame->setObjectName("toolbarFrame");

            auto *bar = new QHBoxLayout(toolbarFrame);
            bar->setContentsMargins(12, 8, 12, 8);
            bar->setSpacing(1);

            auto makeTextBtn = [&](const QString &text, const QString &obj) -> QPushButton* {
                auto *b = new QPushButton(text, toolbarFrame);
            b->setObjectName(obj);
            b->setFocusPolicy(Qt::NoFocus);
            b->setCursor(Qt::PointingHandCursor);
            b->setFlat(true);
            b->setMinimumHeight(30);
            return b;
        };

        toolsBtn   = makeTextBtn("🧰 Tools", "toolbarTextBtn");
        findBtn    = makeTextBtn("🔎 Find",  "toolbarTextBtn");
        aboutBtn   = makeTextBtn("🙎🏻‍♂️ About", "toolbarTextBtn");
        historyBtn = makeTextBtn("", "toolbarTextBtn");
        historyBtn->setToolTip("History");
        historyBtn->setFixedSize(52, 36);

        historyBtn->setIcon(QIcon("history.svg"));
        historyBtn->setIconSize(QSize(20,20));

        auto *sep1 = new QLabel("|", toolbarFrame);
        auto *sep2 = new QLabel("|", toolbarFrame);

        sep1->setObjectName("toolbarSeparator");
        sep2->setObjectName("toolbarSeparator");

        bar->addWidget(toolsBtn);
        bar->addWidget(sep1);
        bar->addWidget(findBtn);
        bar->addWidget(sep2);
        bar->addWidget(aboutBtn);
        bar->addStretch();
        bar->addWidget(historyBtn);

        innerLayout->addWidget(toolbarFrame);

        connect(toolsBtn,   &QPushButton::clicked, this, [this]() { showToolsMenu(); });
        connect(findBtn,    &QPushButton::clicked, this, [this]() { toggleSearchBar(); });
        connect(aboutBtn,   &QPushButton::clicked, this, [this]() { showAboutPanel(); });
        connect(historyBtn, &QPushButton::clicked, this, &OS1Calculator::toggleHistory);
    }

    // ── Helper: show/hide search bar ─────────
    void toggleSearchBar() {
        if (!quickSearch) return;
        if (quickSearch->isVisible()) {
            quickSearch->hide();
            searchTarget.clear();
        } else {
            quickSearch->show();
            quickSearch->raise();
            quickSearch->setFocus();
            quickSearch->clear();
        }
    }

    void buildDisplayCard(QVBoxLayout *innerLayout, QWidget *innerContent) {
        auto *displayCard = new QWidget(innerContent);
        displayCard->setObjectName("displayCard");
        displayCard->setFixedHeight(110);

        auto *dl = new QVBoxLayout(displayCard);
        dl->setContentsMargins(15, 8, 15, 8);
        dl->setSpacing(2);

        expressionLabel = new QLabel("", displayCard);
        expressionLabel->setCursor(Qt::PointingHandCursor);
        expressionLabel->setObjectName("expressionLabel");
        expressionLabel->setAlignment(Qt::AlignRight);
        expressionLabel->installEventFilter(this);

        display = new QLineEdit("0", displayCard);
        QFont df;
        df.setFamily("Segoe UI");
        df.setPointSize(30);
        df.setWeight(QFont::Light);
        display->setFont(df);
        display->setReadOnly(true);
        display->setObjectName("displayBox");
        display->setAlignment(Qt::AlignRight);
        display->installEventFilter(this);

        connect(display, &QLineEdit::textChanged, this, [this](const QString &t) {
            QFont f = display->font();
            int l = t.length();
            f.setPointSize(l <= 9 ? 30 : l <= 14 ? 24 : l <= 19 ? 18 : l <= 24 ? 14 : 10);
            display->setFont(f);
        });

        display->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(display, &QLineEdit::customContextMenuRequested, this, [this]() {
            QApplication::clipboard()->setText(display->text());
            QToolTip::showText(display->mapToGlobal(display->rect().topLeft()), "Copied!");
        });

        dl->addWidget(expressionLabel);
        dl->addWidget(display);
        innerLayout->addWidget(displayCard);
    }

    void buildSearchBar(QVBoxLayout *innerLayout, QWidget *innerContent) {
        quickSearch = new QLineEdit(innerContent);

        QStringList searchItems = {
            "Triangle","Rectangle","Square","Circle",
            "Cube","Sphere","Cylinder","Cone",
            "Area","Volume","Perimeter","Circumference",
            "Temperature","Mass","Weight","Length",
            "Kilogram","Gram","Foot","Feet","Inch",
            "Metre","Kilometre","Mile",
            "USD","INR","EUR","GBP",
            "EMI","Loan","Interest","GST","Tax","CGST","SGST","IGST"
        };
        QCompleter *completer = new QCompleter(searchItems, this);
        completer->setCaseSensitivity(Qt::CaseInsensitive);
        completer->setFilterMode(Qt::MatchContains);
        completer->popup()->setStyleSheet(
            "background:#2C2C2E;"
            "color:white;"
            "font-size:13px;"
            "selection-background-color:#F2730C;"
            );
        quickSearch->setCompleter(completer);
        quickSearch->setPlaceholderText(
            "Search: triangle, circle, km to mile, EMI, GST, celsius..."
            );
        quickSearch->setFixedHeight(32);
        quickSearch->installEventFilter(this);

        connect(quickSearch, &QLineEdit::textChanged, this, [this](const QString &text) {
            QString q = text.trimmed().toLower();

            if (q.isEmpty()) {
                searchTarget.clear();
                if (current_view_mode == MODE_GEOMETRY ||
                    current_view_mode == MODE_UNIT     ||
                    current_view_mode == MODE_CURRENCY ||
                    current_view_mode == MODE_EMI      ||
                    current_view_mode == MODE_GST) {
                    current_view_mode = MODE_STANDARD;
                    renderActiveMatrix();
                }
                return;
            }

            static const QMap<QString,QString> aliases = {
                {"temp","temperature"}, {"tmp","temperature"},
                {"tri","triangle"},     {"tringle","triangle"}, {"traingle","triangle"},
                {"rect","rectangle"},   {"cir","circle"},
                {"sph","sphere"},       {"cub","cube"},
                {"kg","kilogram"},      {"gm","gram"},
                {"ft","foot"},          {"in","inch"},
                {"wt","weight"},        {"mss","mass"},
                {"loan","emi"},         {"interest","emi"},
                {"tax","gst"},          {"cgst","gst"},  {"sgst","gst"}, {"igst","gst"}
            };
            if (aliases.contains(q)) q = aliases[q];

            searchTarget = q;

            // EMI / GST keywords
            if (q.contains("emi") || q.contains("loan") || q.contains("interest") || q.contains("mortgage")) {
                current_view_mode = MODE_EMI;
                renderActiveMatrix();
                return;
            }
            if (q.contains("gst") || q.contains("tax") || q.contains("cgst") || q.contains("sgst") || q.contains("igst")) {
                current_view_mode = MODE_GST;
                renderActiveMatrix();
                return;
            }

            // Geometry shapes
            static const QStringList geoShapes = {
                "triangle","square","rectangle","circle","cube","sphere",
                "cylinder","cone","ellipse","trapezoid","parallelogram",
                "rhombus","hexagon","pentagon","sector","hemisphere",
                "torus","prism","pyramid","cuboid"
            };
            for (const QString &shape : geoShapes) {
                if (q.contains(shape) || shape.contains(q)) {
                    current_view_mode = MODE_GEOMETRY;
                    renderActiveMatrix();
                    return;
                }
            }

            // Unit keywords
            static const QStringList unitKeywords = {
                "inch","feet","foot","meter","metre","km","mile",
                "kg","gram","pound","ounce","celsius","fahrenheit",
                "kelvin","litre","liter","gallon","speed","time",
                "energy","power","pressure","data","angle","fuel",
                "length","mass","weight","area","volume","temperature"
            };
            for (const QString &unit : unitKeywords) {
                if (q.contains(unit) || unit.contains(q)) {
                    current_view_mode = MODE_UNIT;
                    renderActiveMatrix();
                    return;
                }
            }

            // Currency keywords
            static const QStringList currencyKeywords = {
                "currency","usd","inr","eur","gbp","jpy",
                "dollar","rupee","euro","pound","yen"
            };
            for (const QString &cur : currencyKeywords) {
                if (q.contains(cur) || cur.contains(q)) {
                    current_view_mode = MODE_CURRENCY;
                    renderActiveMatrix();
                    return;
                }
            }

            // Generic geometry fallback
            if (q.contains("area") || q.contains("volume") ||
                q.contains("surface") || q.contains("perimeter") ||
                q.contains("circumference")) {
                current_view_mode = MODE_GEOMETRY;
                renderActiveMatrix();
            }
        });

        innerLayout->addWidget(quickSearch);
        quickSearch->hide(); // hidden by default
    }

    void buildHistoryDrawer(QVBoxLayout *innerLayout, QWidget *innerContent) {
        historyWidget = new QWidget(innerContent);
        historyWidget->setObjectName("historyWidget");
        auto *hv = new QVBoxLayout(historyWidget);
        hv->setContentsMargins(0, 0, 0, 0);
        hv->setSpacing(4);

        auto *clearBtn = new QPushButton("🗑  Clear History", historyWidget);
        clearBtn->setObjectName("clearHistoryBtn");
        clearBtn->setFocusPolicy(Qt::NoFocus);
        hv->addWidget(clearBtn);

        // History-only search (separate from Find)
        historySearchBox = new QLineEdit(historyWidget);
        historySearchBox->setPlaceholderText("Search history: date, time, expression, result…");
        historySearchBox->setFixedHeight(30);
        historySearchBox->hide();                       // shown via Ctrl+F when panel focused
        hv->addWidget(historySearchBox);

        connect(historySearchBox, &QLineEdit::textChanged, this, [this](const QString &t) {
            rebuildHistoryList(t);
        });

        historyList = new QListWidget(historyWidget);
        historyList->setObjectName("historyList");
        historyList->setMinimumHeight(260);
        historyList->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        historyList->setFocusPolicy(Qt::ClickFocus);    // so the panel can "have focus"
        hv->addWidget(historyList);

        historyWidget->hide();
        innerLayout->addWidget(historyWidget, 1);

        connect(historyList, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *item) {
            QString t = item->data(Qt::UserRole).toString();
            if (t.isEmpty()) t = item->text();
            int idx = t.lastIndexOf("= ");
            if (idx != -1) {
                QString res = t.mid(idx + 2).trimmed();
                // strip a trailing "(hh:mm AP)" if present
                res = res.section("  ", 0, 0).trimmed();
                display->setText(res);
                current_val = res.remove(",").toDouble();

                last_op.clear();
                start_new_num = true;
                expressionLabel->clear();
                display->setFocus();
                toggleHistory();
            }
        });

        connect(clearBtn, &QPushButton::clicked, this, [this]() {
            HistoryManager::instance()->clearHistory();
        });
    }

    // ─────────────────────────────────────────
    //  MENUS & MODES
    // ─────────────────────────────────────────
    void closeToolsMenu() {
        if (themesSubmenu) { themesSubmenu->close(); themesSubmenu->deleteLater(); themesSubmenu = nullptr; }
        if (toolsPopup)    { toolsPopup->close();    toolsPopup->deleteLater();    toolsPopup = nullptr; }
    }

    void switchMode(int mode) {
        if (mode == MODE_SCIENTIFIC || mode == MODE_STANDARD) {
            current_view_mode = mode;
        } else {
            current_view_mode = (current_view_mode == mode) ? MODE_STANDARD : mode;
        }
        searchTarget.clear();
        renderActiveMatrix();
        closeToolsMenu();
    }

    void showToolsMenu() {
        if (toolsPopup) { closeToolsMenu(); return; }

        toolsPopup = new QWidget(this, Qt::Popup);
        toolsPopup->setObjectName("menuPopup");
        auto *lay = new QVBoxLayout(toolsPopup);
        lay->setContentsMargins(6, 6, 6, 6);
        lay->setSpacing(2);

        auto addItem = [&](const QString &text, std::function<void()> fn) -> QPushButton* {
            auto *b = new QPushButton(text, toolsPopup);
            b->setObjectName("menuItem");
            b->setFlat(true);
            b->setMinimumHeight(30);
            b->setStyleSheet("QPushButton{text-align:left;padding-left:10px;border:none;"
                             "border-radius:6px;font-size:13px;}"
                             "QPushButton:hover{background:rgba(242,115,12,160);color:white;}");
            connect(b, &QPushButton::clicked, this, [fn]() { fn(); });
            lay->addWidget(b);
            return b;
        };

        QString modeLabel = (current_view_mode == MODE_SCIENTIFIC) ? "Standard" : "Scientific";
        int     modeTarget = (current_view_mode == MODE_SCIENTIFIC) ? MODE_STANDARD : MODE_SCIENTIFIC;

        addItem(modeLabel,        [this, modeTarget]() { switchMode(modeTarget); });
        addItem("Currency",       [this]() { switchMode(MODE_CURRENCY); });
        addItem("Geometry",       [this]() { switchMode(MODE_GEOMETRY); });
        addItem("Unit Converter", [this]() { switchMode(MODE_UNIT); });
        addItem("EMI Calculator", [this]() { switchMode(MODE_EMI); });
        addItem("GST Calculator", [this]() { switchMode(MODE_GST); });

        auto *themesItem = addItem("Themes  ▸", [this]() { toggleThemesSubmenu(); });
        themesItem->setObjectName("themesMenuItem");

        toolsPopup->setStyleSheet(
            "#menuPopup{background:rgba(40,40,44,245);border:1px solid rgba(255,255,255,0.12);"
            "border-radius:10px;}");
        toolsPopup->adjustSize();
        toolsPopup->resize(220, toolsPopup->height());
        toolsPopup->move(toolsBtn->mapToGlobal(QPoint(0, toolsBtn->height() + 4)));
        toolsPopup->show();
    }

    void toggleThemesSubmenu() {
        if (themesSubmenu) {
            themesSubmenu->close();
            themesSubmenu->deleteLater();
            themesSubmenu = nullptr;
            return;
        }
        if (!toolsPopup) return;

        themesSubmenu = new QWidget(this, Qt::Popup);
        themesSubmenu->setObjectName("menuPopup");
        auto *row = new QHBoxLayout(themesSubmenu);
        row->setContentsMargins(8, 6, 8, 6);
        row->setSpacing(6);

        auto check = [this](int t) { return ThemeService::instance()->getThemeSetting() == t ? "  ✅" : ""; };
        auto addThemeBtn = [&](const QString &label, int t) {
            auto *b = new QPushButton(label + check(t), themesSubmenu);
            b->setFlat(true);
            b->setMinimumHeight(30);
            b->setStyleSheet("QPushButton{border:none;border-radius:6px;padding:0 8px;font-size:13px;}"
                             "QPushButton:hover{background:rgba(242,115,12,160);color:white;}");
            connect(b, &QPushButton::clicked, this, [this, t]() {
                ThemeService::instance()->setThemeSetting(t);
                closeToolsMenu();
            });
            row->addWidget(b);
        };

        addThemeBtn("☀︎ Light",  0);
        addThemeBtn("☼ Dark",   1);
        addThemeBtn("⚙️ System", 2);

        themesSubmenu->setStyleSheet(
            "#menuPopup{background:rgba(40,40,44,250);border:1px solid rgba(255,255,255,0.14);"
            "border-radius:10px;}");
        themesSubmenu->adjustSize();
        QPushButton *themeBtn =
            toolsPopup->findChild<QPushButton*>("themesMenuItem");

        if (themeBtn) {
            QPoint p = themeBtn->mapToGlobal(
                QPoint(themeBtn->width() + 2, -5)
                );

            themesSubmenu->move(p);
        }

        themesSubmenu->show();
        themesSubmenu->raise();
    }

    void showAboutPanel() {
        if (aboutPopup) {
            aboutPopup->close(); aboutPopup->deleteLater(); aboutPopup = nullptr;
            aboutEmailLabel = nullptr; aboutPhotoLabel = nullptr; aboutLoginBtn = nullptr;
            return;
        }

        aboutPopup = new QWidget(this, Qt::Popup);
        aboutPopup->setObjectName("aboutPopup");
        auto *lay = new QVBoxLayout(aboutPopup);
        lay->setContentsMargins(16, 16, 16, 16);
        lay->setSpacing(10);

        auto *title = new QLabel("About", aboutPopup);
        title->setStyleSheet("font-size:16px;font-weight:bold;");
        title->setAlignment(Qt::AlignCenter);
        lay->addWidget(title);

        auto *photo = new QLabel(aboutPopup);
        photo->setFixedSize(72, 72);
        photo->setAlignment(Qt::AlignCenter);
        photo->setStyleSheet("font-size:34px;background:rgba(120,120,124,120);border-radius:36px;");
        auto *photoRow = new QHBoxLayout();
        photoRow->addStretch(); photoRow->addWidget(photo); photoRow->addStretch();
        lay->addLayout(photoRow);
        aboutPhotoLabel = photo;

        auto *email = new QLabel("Not signed in", aboutPopup);
        email->setAlignment(Qt::AlignCenter);
        email->setWordWrap(true);
        email->setStyleSheet("color:#8E8E93;font-size:12px;");
        lay->addWidget(email);
        aboutEmailLabel = email;

        auto *loginBtn = new QPushButton("Sign in with Google", aboutPopup);
        loginBtn->setObjectName("actionButton");
        loginBtn->setFixedHeight(34);
        lay->addWidget(loginBtn);
        aboutLoginBtn = loginBtn;

        connect(loginBtn, &QPushButton::clicked, this, [this]() {
            if (googleAuth && googleAuth->isSignedIn()) {
                googleAuth->signOut();
                QToolTip::showText(QCursor::pos(), "Signed out");
            } else if (googleAuth) {
                QToolTip::showText(QCursor::pos(), "Opening browser to sign in…");
                googleAuth->signIn();
            }
        });

        auto *backupBtn = new QPushButton("Backup Now", aboutPopup);
        backupBtn->setFixedHeight(32);
        connect(backupBtn, &QPushButton::clicked, this, [this]() {
            performBackupNow();
        });
        lay->addWidget(backupBtn);

        auto *restoreBtn = new QPushButton("Restore from Drive", aboutPopup);
        restoreBtn->setFixedHeight(32);
        connect(restoreBtn, &QPushButton::clicked, this, [this]() {
            performRestoreNow();
        });
        lay->addWidget(restoreBtn);

        auto *statusLabel = new QLabel(backupStatusText(), aboutPopup);
        statusLabel->setObjectName("backupStatusLabel");
        statusLabel->setAlignment(Qt::AlignCenter);
        statusLabel->setWordWrap(true);
        statusLabel->setStyleSheet("color:#8E8E93;font-size:10px;");
        lay->addWidget(statusLabel);

        auto *autoRow = new QHBoxLayout();
        autoRow->addWidget(new QLabel("Auto Backup:", aboutPopup));
        auto *autoCombo = new QComboBox(aboutPopup);
        autoCombo->addItems({"Off", "1 Hour", "Daily", "Weekly", "Monthly"});
        autoCombo->setCurrentText(QSettings("1OS","Calculator").value("AutoBackup","Daily").toString());
        connect(autoCombo, &QComboBox::currentTextChanged, this, [this](const QString &v) {
            QSettings("1OS","Calculator").setValue("AutoBackup", v);
            setupAutoBackupTimer();
        });
        autoRow->addWidget(autoCombo);
        lay->addLayout(autoRow);

        auto *footer = new QLabel("Powered By 1 Company", aboutPopup);
        footer->setAlignment(Qt::AlignCenter);
        footer->setStyleSheet("color:#8E8E93;font-size:10px;margin-top:6px;");
        lay->addWidget(footer);

        aboutPopup->setStyleSheet(
            "#aboutPopup{background:rgba(40,40,44,248);border:1px solid rgba(255,255,255,0.12);"
            "border-radius:12px;color:#E5E5EA;}"
            "QComboBox{background:#3A3A3C;color:white;border-radius:4px;padding-left:6px;min-height:28px;}");
        aboutPopup->setFixedWidth(260);
        aboutPopup->adjustSize();
        aboutPopup->move(aboutBtn->mapToGlobal(QPoint(0, aboutBtn->height() + 4)));
        aboutPopup->show();

        refreshAboutSignInUi();
    }

    // ─────────────────────────────────────────
    //  GOOGLE SIGN-IN / BACKUP UI HELPERS
    // ─────────────────────────────────────────
    QString backupStatusText() const {
        QString last = GoogleDriveBackup::lastBackupTime();
        QString lastR = GoogleDriveBackup::lastRestoreTime();
        QStringList parts;
        if (!last.isEmpty()) {
            QDateTime dt = QDateTime::fromString(last, Qt::ISODate);
            parts << "Last backup: " + (dt.isValid() ? dt.toString("dd MMM, hh:mm AP") : last);
        }
        if (!lastR.isEmpty()) {
            QDateTime dt = QDateTime::fromString(lastR, Qt::ISODate);
            parts << "Last restore: " + (dt.isValid() ? dt.toString("dd MMM, hh:mm AP") : lastR);
        }
        if (parts.isEmpty()) return "No backups yet";
        return parts.join("\n");
    }

    // Updates the live About-panel widgets (if open) to reflect current
    // sign-in state. Safe to call even if aboutPopup is currently closed.
    void refreshAboutSignInUi() {
        if (!googleAuth) return;
        bool signedIn = googleAuth->isSignedIn() && !googleAuth->userEmail().isEmpty();

        if (aboutEmailLabel) {
            aboutEmailLabel->setText(signedIn
                ? (googleAuth->userName().isEmpty() ? googleAuth->userEmail()
                                                      : googleAuth->userName() + "\n" + googleAuth->userEmail())
                : "Not signed in");
        }
        if (aboutLoginBtn) {
            aboutLoginBtn->setText(signedIn ? "Sign out" : "Sign in with Google");
        }
        if (aboutPhotoLabel) {
            if (signedIn && googleAuth->hasPicture()) {
                QPixmap rounded = googleAuth->userPicture().scaled(
                    72, 72, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
                aboutPhotoLabel->setPixmap(rounded);
                aboutPhotoLabel->setText(QString());
            } else {
                aboutPhotoLabel->setPixmap(QPixmap());
                aboutPhotoLabel->setText("👤");
            }
        }
        if (aboutPopup) {
            QLabel *statusLbl = aboutPopup->findChild<QLabel*>("backupStatusLabel");
            if (statusLbl) statusLbl->setText(backupStatusText());
        }
    }

    // Serializes calculator history + relevant settings into a single
    // JSON object suitable for uploading to Google Drive appData.
    QJsonObject buildBackupPayload() const {
        QJsonObject root;
        root["schemaVersion"] = 1;
        root["savedAt"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

        root["history"] = HistoryManager::instance()->getEntriesAsJson();

        QJsonObject settingsObj;
        QSettings s("1OS", "Calculator");
        settingsObj["ThemeMode"]   = ThemeService::instance()->getThemeSetting();
        settingsObj["AutoBackup"]  = s.value("AutoBackup", "Daily").toString();
        root["settings"] = settingsObj;

        return root;
    }

    // Applies a previously-downloaded backup payload: replaces history
    // and restores settings, then refreshes the UI in place.
    void applyRestoredPayload(const QJsonObject &payload) {
        QJsonArray histArr = payload.value("history").toArray();
        HistoryManager::instance()->setEntriesFromJson(histArr);

        QJsonObject settingsObj = payload.value("settings").toObject();
        QSettings s("1OS", "Calculator");
        if (settingsObj.contains("ThemeMode")) {
            ThemeService::instance()->setThemeSetting(settingsObj.value("ThemeMode").toInt(2));
        }
        if (settingsObj.contains("AutoBackup")) {
            s.setValue("AutoBackup", settingsObj.value("AutoBackup").toString());
        }

        setupAutoBackupTimer();
    }

    void performBackupNow() {
        if (!googleAuth || !driveBackup) return;
        if (!googleAuth->isSignedIn()) {
            QToolTip::showText(QCursor::pos(), "Sign in with Google first");
            return;
        }
        if (backupInFlight) return;
        backupInFlight = true;
        QToolTip::showText(QCursor::pos(), "Backing up…");

        QJsonObject payload = buildBackupPayload();
        driveBackup->backupNow(payload, [this](bool ok, QString err) {
            backupInFlight = false;
            if (ok) {
                QToolTip::showText(QCursor::pos(), "Backed up to Google Drive");
            } else {
                QToolTip::showText(QCursor::pos(), "Backup failed: " + err);
            }
            refreshAboutSignInUi();
        });
    }

    void performRestoreNow() {
        if (!googleAuth || !driveBackup) return;
        if (!googleAuth->isSignedIn()) {
            QToolTip::showText(QCursor::pos(), "Sign in with Google first");
            return;
        }
        if (backupInFlight) return;
        backupInFlight = true;
        QToolTip::showText(QCursor::pos(), "Restoring…");

        driveBackup->restoreNow([this](bool ok, QJsonObject payload, QString err) {
            backupInFlight = false;
            if (ok) {
                applyRestoredPayload(payload);
                QToolTip::showText(QCursor::pos(), "Restored from Google Drive");
            } else {
                QToolTip::showText(QCursor::pos(), "Restore failed: " + err);
            }
            refreshAboutSignInUi();
        });
    }

    // Re-creates the auto-backup QTimer based on the current
    // AutoBackup QSettings value ("Off" / "1 Hour" / "Daily" / "Weekly" / "Monthly").
    void setupAutoBackupTimer() {
        if (autoBackupTimer) {
            autoBackupTimer->stop();
            autoBackupTimer->deleteLater();
            autoBackupTimer = nullptr;
        }

        QString mode = QSettings("1OS", "Calculator").value("AutoBackup", "Daily").toString();
        qint64 intervalMs = 0;
        if (mode == "1 Hour")      intervalMs = 60LL * 60 * 1000;
        else if (mode == "Daily")  intervalMs = 24LL * 60 * 60 * 1000;
        else if (mode == "Weekly") intervalMs = 7LL * 24 * 60 * 60 * 1000;
        else if (mode == "Monthly")intervalMs = 30LL * 24 * 60 * 60 * 1000;
        else return; // "Off" or unrecognised → no timer

        autoBackupTimer = new QTimer(this);
        connect(autoBackupTimer, &QTimer::timeout, this, [this]() {
            if (googleAuth && googleAuth->isSignedIn()) {
                performBackupNow();
            }
        });
        autoBackupTimer->start(static_cast<int>(qMin<qint64>(intervalMs, INT_MAX)));
    }

    // ─────────────────────────────────────────
    //  HISTORY HELPERS
    // ─────────────────────────────────────────
    void toggleHistorySearch() {
        if (!historySearchBox) return;
        history_search_visible = !history_search_visible;
        historySearchBox->setVisible(history_search_visible);
        if (history_search_visible) {
            historySearchBox->setFocus();
            historySearchBox->clear();
        } else {
            historySearchBox->clear();
            rebuildHistoryList(QString());
        }
    }

    void rebuildHistoryList(const QString &query) {
        if (!historyList) return;
        historyList->clear();
        QString q = query.trimmed().toLower();

        QString currentHeader;
        for (const HistEntry &e : HistoryManager::instance()->getEntries()) {
            if (!HistoryManager::instance()->histMatches(e, q)) continue;
            QString header = HistoryManager::instance()->dateHeaderFor(e.when);
            if (header != currentHeader) {
                currentHeader = header;
                auto *hdr = new QListWidgetItem(header + "\n────────────");
                hdr->setFlags(Qt::NoItemFlags);
                QFont hf = hdr->font();
                hf.setBold(true);
                hdr->setForeground(QColor("#FF9F0A"));
                hdr->setFont(hf);
                historyList->addItem(hdr);
            }

            QString time = e.when.isValid() ? e.when.toString("(hh:mm AP)") : "";
            auto *it = new QListWidgetItem(e.text + (time.isEmpty() ? "" : "   " + time));
            it->setData(Qt::UserRole, e.text);
            historyList->addItem(it);
        }
    }

    // ─────────────────────────────────────────
    //  TOGGLE HISTORY
    // ─────────────────────────────────────────
    void toggleHistory() {
        is_history_visible = !is_history_visible;
        historyWidget->setVisible(is_history_visible);
        gridContainer->setVisible(!is_history_visible);
        display->setVisible(true);
        if (current_view_mode != MODE_CURRENCY)
            resize(width(), is_history_visible ? 640 : 560);
    }

    // ─────────────────────────────────────────
    //  MAXIMIZE TOGGLE
    // ─────────────────────────────────────────
    void handleMaximizeToggle() {
        if (!isMaximizedState) {
            resize(520, 720);
            isMaximizedState = true;
            btnMax->setToolTip("Restore");
        } else {
            resize(360, 560);
            isMaximizedState = false;
            btnMax->setToolTip("Maximise");
        }
    }

    // ─────────────────────────────────────────
    //  RENDER MATRIX (main dispatch)
    // ─────────────────────────────────────────
    void renderActiveMatrix() {
        QLayoutItem *ci;
        while ((ci = buttonGrid->takeAt(0)) != nullptr) {
            if (ci->widget()) {
                ci->widget()->hide();
                ci->widget()->deleteLater();
            }
            delete ci;
        }

        if (current_view_mode != MODE_CURRENCY) {
            fromCurrencyCombo   = nullptr;
            toCurrencyCombo     = nullptr;
            currencyInput       = nullptr;
            currencyResultLabel = nullptr;
        }

        // Display visible for all modes except currency
        // Display visible for all modes
        display->setVisible(true);
        switch (current_view_mode) {
        case MODE_STANDARD:   renderStandard();       break;
        case MODE_SCIENTIFIC: renderScientific();     break;
        case MODE_CURRENCY:   renderCurrency();       break;
        case MODE_GEOMETRY:   renderGeometry();       break;
        case MODE_UNIT:       renderUnitConverter();  break;
        case MODE_EMI:        renderEMI();            break;
        case MODE_GST:        renderGST();            break;
        }
        Apply1OSStyleSheet();
    }

    QPushButton* makeBtn(const QString &label, const QString &objName = "sciButton") {
        auto *b = new QPushButton(label, gridContainer);
        b->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        b->setFocusPolicy(Qt::NoFocus);
        b->setObjectName(objName);
        b->show();
        connect(b, &QPushButton::clicked, this, &OS1Calculator::handleButtonClick);
        return b;
    }

    QString btnObjName(const QString &lbl) {
        if (lbl == "C" || lbl == "CE" || lbl == "DEL" || lbl == "AC") return "sysActionButton";
        if (lbl == "/" || lbl == "x"  || lbl == "-"   || lbl == "+" || lbl == "=") return "actionButton";
        if ((lbl >= "0" && lbl <= "9") || lbl == "." || lbl == "00" || lbl == "000") return "numButton";
        return "sciButton";
    }

    // ─────────────────────────────────────────
    //  STANDARD CALCULATOR
    // ─────────────────────────────────────────
    void renderStandard() {
        if (!isMaximizedState) resize(360, 560);

        const char *keys[6][4] = {
            {"%",   "CE",   "C",   "DEL"},
            {"1/x", "x^2",  "sqrt","/"},
            {"7",   "8",    "9",   "x"},
            {"4",   "5",    "6",   "-"},
            {"1",   "2",    "3",   "+"},
            {"+/-", "0",    ".",   "="}
        };
        for (int r = 0; r < 6; r++)
            for (int c = 0; c < 4; c++) {
                QString lbl = keys[r][c];
                buttonGrid->addWidget(makeBtn(lbl, btnObjName(lbl)), r, c);
            }
    }

    // ─────────────────────────────────────────
    //  SCIENTIFIC CALCULATOR
    // ─────────────────────────────────────────
    void renderScientific() {
        if (!isMaximizedState) resize(430, 620);

        auto *degRadBtn = new QPushButton(isDegMode ? "DEG" : "RAD", gridContainer);
        degRadBtn->setObjectName("sciButton");
        degRadBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        degRadBtn->setFocusPolicy(Qt::NoFocus);
        degRadBtn->show();
        connect(degRadBtn, &QPushButton::clicked, this, [this, degRadBtn]() {
            isDegMode = !isDegMode;
            degRadBtn->setText(isDegMode ? "DEG" : "RAD");
        });
        buttonGrid->addWidget(degRadBtn, 0, 0);

        auto *sndBtn = new QPushButton("2nd", gridContainer);
        sndBtn->setObjectName("sciButton");
        sndBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        sndBtn->setFocusPolicy(Qt::NoFocus);
        sndBtn->show();
        connect(sndBtn, &QPushButton::clicked, this, [this]() {
            is2ndMode = !is2ndMode;
            renderActiveMatrix();
        });
        buttonGrid->addWidget(sndBtn, 0, 1);

        QString row0[] = {"C", "DEL", "+/-", "/"};
        for (int c = 0; c < 4; c++)
            buttonGrid->addWidget(makeBtn(row0[c], btnObjName(row0[c])), 0, c + 2);

        struct SciKey { QString lbl, lbl2; };
        SciKey row1[] = {{"x^2","x^3"},{"x^y","y√x"},{"e^x","10^x"},{"ln","log"},{"1/x","x!"},{"x",""}};
        SciKey row2[] = {{"sin","asin"},{"cos","acos"},{"tan","atan"},{"7",""},{"8",""},{"9",""}};
        SciKey row3[] = {{"sinh","asinh"},{"cosh","acosh"},{"tanh","atanh"},{"4",""},{"5",""},{"6",""}};
        SciKey row4[] = {{"pi","e"},{"√","∛"},{"abs","|mod|"},{"1",""},{"2",""},{"3",""}};
        SciKey row5[] = {{"00","000"},{"EE","Rand"},{"0",""},{".",""},{"+",""},{"=",""}};

        auto addSciRow = [&](SciKey *keys, int count, int row) {
            for (int c = 0; c < count; c++) {
                QString lbl = (is2ndMode && !keys[c].lbl2.isEmpty()) ? keys[c].lbl2 : keys[c].lbl;
                buttonGrid->addWidget(makeBtn(lbl, btnObjName(lbl)), row, c);
            }
        };

        addSciRow(row1, 6, 1);
        addSciRow(row2, 6, 2);
        addSciRow(row3, 6, 3);
        addSciRow(row4, 6, 4);
        addSciRow(row5, 6, 5);

        SciKey rowB[] = {{"(",""},{")",""},{"-",""},{"x",""}};
        for (int c = 0; c < 4; c++)
            buttonGrid->addWidget(makeBtn(rowB[c].lbl, btnObjName(rowB[c].lbl)), 6, c + 2);
        buttonGrid->addWidget(makeBtn("0", "numButton"),    6, 0);
        buttonGrid->addWidget(makeBtn("=", "actionButton"), 6, 1);
    }

    // ─────────────────────────────────────────
    //  CURRENCY CONVERTER
    // ─────────────────────────────────────────
    void renderCurrency() {
        if (!isMaximizedState) resize(360, 390);

        auto *titleLbl = new QLabel("Currency Converter", gridContainer);
        titleLbl->setStyleSheet("font-size:13px;font-weight:bold;color:#8E8E93;");
        buttonGrid->addWidget(titleLbl, 0, 0, 1, 3);

        auto *infoLbl = new QLabel(
            QString("Source: %1   Updated: %2")
                .arg(CurrencyService::instance()->getRateSource())
                .arg(CurrencyService::instance()->getLastRateUpdate().isEmpty() ? "--" : CurrencyService::instance()->getLastRateUpdate()),
            gridContainer);
        infoLbl->setStyleSheet("color:#888;font-size:9px;");
        buttonGrid->addWidget(infoLbl, 1, 0, 1, 3);

        currencyInput = new QLineEdit("1", gridContainer);
        currencyInput->setStyleSheet(
            "height:40px;font-size:22px;color:white;"
            "background:#2C2C2E;border-radius:6px;padding-left:8px;font-weight:bold;");
        buttonGrid->addWidget(currencyInput, 2, 0, 1, 3);

        fromCurrencyCombo = new QComboBox(gridContainer);
        toCurrencyCombo   = new QComboBox(gridContainer);

        for (auto *cb : {fromCurrencyCombo, toCurrencyCombo}) {
            cb->setEditable(true);
            cb->setInsertPolicy(QComboBox::NoInsert);
            cb->addItems({
                "USD ($)", "INR (₹)", "EUR (€)", "GBP (£)", "JPY (¥)", "CNY (¥)", "KRW (₩)", "RUB (₽)", "TRY (₺)", "VND (₫)",
                "THB (฿)", "PHP (₱)", "BDT (৳)", "ILS (₪)", "NGN (₦)", "AUD ($)", "CAD ($)", "NZD (NZ$)", "SGD (S$)", "HKD (HK$)",
                "TWD (NT$)", "PKR (₨)", "NPR (₨)", "LKR (₨)", "AED (د.إ)", "SAR (﷼)", "QAR (﷼)", "OMR (﷼)", "KWD (KD)", "BHD (BD)",
                "JOD (JD)", "MYR (RM)", "IDR (Rp)", "KHR (៛)", "LAK (₭)", "MNT (₮)", "KZT (₸)", "AZN (₼)", "GEL (₾)", "UAH (₴)",
                "BYN (Br)", "RON (lei)", "BGN (лв)", "RSD (дин)", "ISK (kr)", "KES (KSh)", "UGX (USh)", "TZS (TSh)", "ETB (Br)",
                "GHS (₵)", "MAD (د.م.)", "DZD (دج)", "EGP (£)", "TND (د.ت)", "BRL (R$)", "ARS ($)", "CLP ($)", "COP ($)", "UYU ($U)",
                "PEN (S/)", "BOB (Bs.)", "PYG (₲)", "CRC (₡)", "GTQ (Q)", "HNL (L)", "NIO (C$)", "DOP ($)", "JMD ($)", "MXN ($)",
                "ZAR (R)", "CHF (CHF)", "SEK (kr)", "NOK (kr)", "DKK (kr)", "PLN (zł)", "CZK (Kč)", "HUF (Ft)", "TTD (TT$)", "BSD ($)", "BBD ($)"
            });
            cb->completer()->setCompletionMode(QCompleter::PopupCompletion);
            cb->completer()->setCaseSensitivity(Qt::CaseInsensitive);
            cb->completer()->setFilterMode(Qt::MatchContains);
            cb->setFocusPolicy(Qt::TabFocus);
            cb->setStyleSheet(
                "height:35px;background:#3A3A3C;color:white;"
                "font-weight:bold;padding-left:5px;border-radius:4px;");
        }
        fromCurrencyCombo->setCurrentText("USD ($)");
        toCurrencyCombo->setCurrentText("INR (₹)");

        auto *swapBtn = new QPushButton("⇄", gridContainer);
        swapBtn->setStyleSheet("font-size:22px;color:#FF9F0A;font-weight:bold;background:transparent;border:none;max-width:40px;");
        swapBtn->setCursor(Qt::PointingHandCursor);
        swapBtn->setFocusPolicy(Qt::NoFocus);

        buttonGrid->addWidget(fromCurrencyCombo, 3, 0);
        buttonGrid->addWidget(swapBtn,           3, 1, Qt::AlignCenter);
        buttonGrid->addWidget(toCurrencyCombo,   3, 2);

        currencyResultLabel = new QLabel("83.50 INR (₹)", gridContainer);
        currencyResultLabel->setStyleSheet(
            "font-size:26px;font-weight:bold;color:#34C759;padding:14px 0;");
        currencyResultLabel->setAlignment(Qt::AlignCenter);
        buttonGrid->addWidget(currencyResultLabel, 4, 0, 1, 3);

        auto calcCur = [this]() {
            if (!currencyInput || !fromCurrencyCombo || !toCurrencyCombo || !currencyResultLabel) return;
            double amount = currencyInput->text().toDouble();
            double res = CalculatorEngine::instance()->convertCurrency(amount, fromCurrencyCombo->currentText(), toCurrencyCombo->currentText());
            QString formattedRes = CalculatorEngine::instance()->formatResult(res);

            currencyResultLabel->setText(
                QString("%1  %2").arg(formattedRes).arg(toCurrencyCombo->currentText()));

            display->setText(formattedRes);
        };

        connect(currencyInput,     &QLineEdit::textChanged,
                this, [=]() { calcCur(); });
        connect(fromCurrencyCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, [=]() { calcCur(); });
        connect(toCurrencyCombo,   QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, [=]() { calcCur(); });
        connect(swapBtn, &QPushButton::clicked, this, [=]() {
            int fi = fromCurrencyCombo->currentIndex();
            int ti = toCurrencyCombo->currentIndex();
            fromCurrencyCombo->setCurrentIndex(ti);
            toCurrencyCombo->setCurrentIndex(fi);
        });

        calcCur();
    }

    // ─────────────────────────────────────────
    //  EMI CALCULATOR
    // ─────────────────────────────────────────
    void renderEMI() {
        if (!isMaximizedState) resize(380, 580);

        auto *scroll = new QScrollArea(gridContainer);
        scroll->setWidgetResizable(true);
        scroll->setStyleSheet("QScrollArea{border:none;background:transparent;}");
        auto *container = new QWidget();
        scroll->setWidget(container);
        auto *vl = new QVBoxLayout(container);
        vl->setSpacing(10);
        vl->setContentsMargins(6, 6, 6, 6);

        // Title
        auto *titleLbl = new QLabel("🏦  EMI Calculator", container);
        titleLbl->setStyleSheet("font-size:15px;font-weight:bold;color:#FF9F0A;margin-bottom:4px;");
        vl->addWidget(titleLbl);

        // ── Input card ───────────────────────
        auto *inputCard = new QWidget(container);
        inputCard->setObjectName("geoCard");
        auto *inputLayout = new QVBoxLayout(inputCard);
        inputLayout->setContentsMargins(10, 10, 10, 10);
        inputLayout->setSpacing(8);

        auto makeField = [&](const QString &labelText, const QString &placeholder,
                             QLineEdit *&outField) {
            auto *row = new QHBoxLayout();
            auto *lbl = new QLabel(labelText, inputCard);
            lbl->setFixedWidth(130);
            lbl->setStyleSheet("font-size:11px;");
            outField = new QLineEdit(placeholder, inputCard);
            outField->setStyleSheet(
                "height:26px;background:#2C2C2E;color:white;"
                "border-radius:4px;padding-left:6px;font-size:12px;");
            row->addWidget(lbl);
            row->addWidget(outField);
            inputLayout->addLayout(row);
        };

        QLineEdit *principalField  = nullptr;
        QLineEdit *rateField       = nullptr;
        QLineEdit *tenureField     = nullptr;
        QComboBox *tenureTypeCombo = nullptr;

        makeField("Loan Amount (₹):",   "500000",  principalField);
        makeField("Annual Interest (%):", "8.5",   rateField);

        // Tenure row with combo
        auto *tenureRow = new QHBoxLayout();
        auto *tenureLbl = new QLabel("Tenure:", inputCard);
        tenureLbl->setFixedWidth(130);
        tenureLbl->setStyleSheet("font-size:11px;");
        tenureField = new QLineEdit("5", inputCard);
        tenureField->setStyleSheet(
            "height:26px;background:#2C2C2E;color:white;"
            "border-radius:4px;padding-left:6px;font-size:12px;");
        tenureTypeCombo = new QComboBox(inputCard);
        tenureTypeCombo->addItems({"Years", "Months"});
        tenureTypeCombo->setStyleSheet(
            "height:28px;background:#3A3A3C;color:white;padding-left:4px;border-radius:4px;font-size:11px;");
        tenureRow->addWidget(tenureLbl);
        tenureRow->addWidget(tenureField);
        tenureRow->addWidget(tenureTypeCombo);
        inputLayout->addLayout(tenureRow);

        vl->addWidget(inputCard);
        inputCard->setStyleSheet("#geoCard{background:rgba(60,60,62,180);border-radius:8px;}");

        // ── Result card ──────────────────────
        auto *resultCard = new QWidget(container);
        resultCard->setObjectName("geoCard");
        auto *resultLayout = new QVBoxLayout(resultCard);
        resultLayout->setContentsMargins(10, 10, 10, 10);
        resultLayout->setSpacing(6);

        auto *emiLbl         = new QLabel("Monthly EMI:  ₹ --", resultCard);
        auto *totalAmtLbl    = new QLabel("Total Amount: ₹ --", resultCard);
        auto *totalIntLbl    = new QLabel("Total Interest: ₹ --", resultCard);
        auto *principalPcLbl = new QLabel("Principal: -- %   Interest: -- %", resultCard);

        for (auto *lbl : {emiLbl, totalAmtLbl, totalIntLbl, principalPcLbl}) {
            lbl->setStyleSheet("font-size:13px;color:#34C759;font-weight:bold;");
            resultLayout->addWidget(lbl);
        }
        emiLbl->setStyleSheet("font-size:18px;color:#FF9F0A;font-weight:bold;");

        resultCard->setStyleSheet("#geoCard{background:rgba(60,60,62,180);border-radius:8px;}");
        vl->addWidget(resultCard);

        // ── Calculate button ─────────────────
        auto *calcBtn = new QPushButton("Calculate EMI", container);
        calcBtn->setObjectName("actionButton");
        calcBtn->setFixedHeight(36);
        vl->addWidget(calcBtn);

        // ── Formula note ─────────────────────
        auto *formulaLbl = new QLabel(
            "Formula: EMI = P × r × (1+r)ⁿ / ((1+r)ⁿ - 1)\n"
            "P=Principal  r=Monthly Rate  n=Months", container);
        formulaLbl->setStyleSheet("font-size:9px;color:#8E8E93;margin-top:4px;");
        vl->addWidget(formulaLbl);
        vl->addStretch();

        // ── Calculation logic ─────────────────
        auto doEMI = [=]() {
            double P = principalField->text().toDouble();
            double annualRate = rateField->text().toDouble();
            int tenureVal = tenureField->text().toInt();
            int n = (tenureTypeCombo->currentIndex() == 0) ? tenureVal * 12 : tenureVal;

            if (P <= 0 || annualRate <= 0 || n <= 0) {
                emiLbl->setText("Monthly EMI: Invalid input");
                return;
            }

            QJsonObject emiResult = CalculatorEngine::instance()->calculateEMI(P, annualRate, n);
            double emi = emiResult["emi"].toDouble();
            double totalAmt = emiResult["totalAmount"].toDouble();
            double totalInt = emiResult["totalInterest"].toDouble();
            double prinPc   = emiResult["principalPercentage"].toDouble();
            double intPc    = emiResult["interestPercentage"].toDouble();

            display->setText(CalculatorEngine::instance()->formatResult(emi));

            auto fmt = [](double v) { return CalculatorEngine::instance()->formatResult(std::round(v)); };

            emiLbl->setText(QString("Monthly EMI:  ₹ %1").arg(fmt(emi)));
            totalAmtLbl->setText(QString("Total Amount: ₹ %1").arg(fmt(totalAmt)));
            totalIntLbl->setText(QString("Total Interest: ₹ %1").arg(fmt(totalInt)));
            principalPcLbl->setText(QString("Principal: %1%   Interest: %2%")
                                        .arg(QString::number(prinPc, 'f', 1))
                                        .arg(QString::number(intPc, 'f', 1)));

            HistoryManager::instance()->addToHistory(QString("EMI ₹%1 @ %2% × %3mo = ₹%4/mo")
                             .arg(fmt(P)).arg(annualRate).arg(n).arg(fmt(emi)));
        };

        connect(calcBtn, &QPushButton::clicked, this, [=]() { doEMI(); });
        connect(principalField, &QLineEdit::returnPressed, this, [=]() { doEMI(); });
        connect(rateField,      &QLineEdit::returnPressed, this, [=]() { doEMI(); });
        connect(tenureField,    &QLineEdit::returnPressed, this, [=]() { doEMI(); });

        auto *wrapper = new QWidget(gridContainer);
        auto *wl = new QVBoxLayout(wrapper);
        wl->setContentsMargins(0, 0, 0, 0);
        wl->addWidget(scroll);
        buttonGrid->addWidget(wrapper, 0, 0);
    }

    // ─────────────────────────────────────────
    //  GST CALCULATOR
    // ─────────────────────────────────────────
    void renderGST() {
        if (!isMaximizedState) resize(380, 560);

        auto *scroll = new QScrollArea(gridContainer);
        scroll->setWidgetResizable(true);
        scroll->setStyleSheet("QScrollArea{border:none;background:transparent;}");
        auto *container = new QWidget();
        scroll->setWidget(container);
        auto *vl = new QVBoxLayout(container);
        vl->setSpacing(10);
        vl->setContentsMargins(6, 6, 6, 6);

        auto *titleLbl = new QLabel("🧾  GST Calculator", container);
        titleLbl->setStyleSheet("font-size:15px;font-weight:bold;color:#FF9F0A;margin-bottom:4px;");
        vl->addWidget(titleLbl);

        // ── Input card ───────────────────────
        auto *inputCard = new QWidget(container);
        inputCard->setObjectName("geoCard");
        auto *inputLayout = new QVBoxLayout(inputCard);
        inputLayout->setContentsMargins(10, 10, 10, 10);
        inputLayout->setSpacing(8);

        // Amount
        auto *amtRow = new QHBoxLayout();
        auto *amtLbl = new QLabel("Amount (₹):", inputCard);
        amtLbl->setFixedWidth(130); amtLbl->setStyleSheet("font-size:11px;");
        auto *amtField = new QLineEdit("1000", inputCard);
        amtField->setStyleSheet("height:26px;background:#2C2C2E;color:white;border-radius:4px;padding-left:6px;font-size:12px;");
        amtRow->addWidget(amtLbl); amtRow->addWidget(amtField);
        inputLayout->addLayout(amtRow);

        // GST rate
        auto *rateRow = new QHBoxLayout();
        auto *rateLbl = new QLabel("GST Rate:", inputCard);
        rateLbl->setFixedWidth(130); rateLbl->setStyleSheet("font-size:11px;");
        auto *rateCombo = new QComboBox(inputCard);
        rateCombo->addItems({"0%", "0.25%", "1%", "1.5%", "3%", "5%", "7.5%", "12%", "18%", "28%"});
        rateCombo->setCurrentText("18%");
        rateCombo->setStyleSheet("height:28px;background:#3A3A3C;color:white;padding-left:4px;border-radius:4px;font-size:12px;");
        auto *customRateField = new QLineEdit("", inputCard);
        customRateField->setPlaceholderText("Custom %");
        customRateField->setStyleSheet("height:26px;background:#2C2C2E;color:white;border-radius:4px;padding-left:6px;font-size:11px;");
        customRateField->setFixedWidth(70);
        rateRow->addWidget(rateLbl); rateRow->addWidget(rateCombo); rateRow->addWidget(customRateField);
        inputLayout->addLayout(rateRow);

        // Mode: Add GST / Remove GST
        auto *modeRow = new QHBoxLayout();
        auto *modeLbl = new QLabel("Mode:", inputCard);
        modeLbl->setFixedWidth(130); modeLbl->setStyleSheet("font-size:11px;");
        auto *modeCombo = new QComboBox(inputCard);
        modeCombo->addItems({"Add GST (Exclusive)", "Remove GST (Inclusive)"});
        modeCombo->setStyleSheet("height:28px;background:#3A3A3C;color:white;padding-left:4px;border-radius:4px;font-size:12px;");
        modeRow->addWidget(modeLbl); modeRow->addWidget(modeCombo);
        inputLayout->addLayout(modeRow);

        vl->addWidget(inputCard);
        inputCard->setStyleSheet("#geoCard{background:rgba(60,60,62,180);border-radius:8px;}");

        // ── Result card ──────────────────────
        auto *resultCard = new QWidget(container);
        resultCard->setObjectName("geoCard");
        auto *resultLayout = new QVBoxLayout(resultCard);
        resultLayout->setContentsMargins(10, 10, 10, 10);
        resultLayout->setSpacing(6);

        auto *baseAmtLbl  = new QLabel("Base Amount:  ₹ --", resultCard);
        auto *gstAmtLbl   = new QLabel("GST Amount:   ₹ --", resultCard);
        auto *cgstLbl     = new QLabel("CGST (50%):   ₹ --", resultCard);
        auto *sgstLbl     = new QLabel("SGST (50%):   ₹ --", resultCard);
        auto *igstLbl     = new QLabel("IGST (100%):  ₹ --", resultCard);
        auto *totalLbl    = new QLabel("Total Amount: ₹ --", resultCard);

        totalLbl->setStyleSheet("font-size:18px;color:#FF9F0A;font-weight:bold;");
        for (auto *lbl : {baseAmtLbl, gstAmtLbl, cgstLbl, sgstLbl, igstLbl, totalLbl}) {
            if (lbl != totalLbl)
                lbl->setStyleSheet("font-size:12px;color:#34C759;font-weight:bold;");
            resultLayout->addWidget(lbl);
        }

        auto *separator = new QLabel("───────────────────", resultCard);
        separator->setStyleSheet("color:#555;font-size:10px;");
        resultLayout->insertWidget(4, separator); // before IGST

        resultCard->setStyleSheet("#geoCard{background:rgba(60,60,62,180);border-radius:8px;}");
        vl->addWidget(resultCard);

        auto *calcBtn = new QPushButton("Calculate GST", container);
        calcBtn->setObjectName("actionButton");
        calcBtn->setFixedHeight(36);
        vl->addWidget(calcBtn);
        vl->addStretch();

        // ── Calculation logic ─────────────────
        auto doGST = [=]() {
            double amount = amtField->text().toDouble();

            // Resolve rate: custom field overrides combo
            double gstRate = 0.0;
            if (!customRateField->text().trimmed().isEmpty()) {
                gstRate = customRateField->text().toDouble();
            } else {
                QString rateStr = rateCombo->currentText();
                rateStr.remove('%');
                gstRate = rateStr.toDouble();
            }

            if (amount <= 0) {
                totalLbl->setText("Total Amount: Invalid input");
                return;
            }

            bool exclusive = (modeCombo->currentIndex() == 0);
            QJsonObject gstResult = CalculatorEngine::instance()->calculateGST(amount, gstRate, exclusive);

            double baseAmt = gstResult["baseAmount"].toDouble();
            double gstAmt = gstResult["gstAmount"].toDouble();
            double totalAmt = gstResult["totalAmount"].toDouble();
            double cgst = gstResult["cgst"].toDouble();
            double sgst = gstResult["sgst"].toDouble();

            display->setText(CalculatorEngine::instance()->formatResult(totalAmt));

            auto fmt = [](double v) { return CalculatorEngine::instance()->formatResult(v); };

            baseAmtLbl->setText(QString("Base Amount:  ₹ %1").arg(fmt(baseAmt)));
            gstAmtLbl->setText(QString("GST Amount (%1%): ₹ %2").arg(gstRate).arg(fmt(gstAmt)));
            cgstLbl->setText(QString("CGST (50%):   ₹ %1").arg(fmt(cgst)));
            sgstLbl->setText(QString("SGST (50%):   ₹ %1").arg(fmt(sgst)));
            igstLbl->setText(QString("IGST (100%):  ₹ %1").arg(fmt(gstAmt)));
            totalLbl->setText(QString("Total Amount: ₹ %1").arg(fmt(totalAmt)));

            HistoryManager::instance()->addToHistory(QString("GST %1%: Base ₹%2 + Tax ₹%3 = ₹%4")
                             .arg(gstRate).arg(fmt(baseAmt)).arg(fmt(gstAmt)).arg(fmt(totalAmt)));
        };

        connect(calcBtn,         &QPushButton::clicked,
                this, [=]() { doGST(); });
        connect(amtField,        &QLineEdit::returnPressed,
                this, [=]() { doGST(); });
        connect(customRateField, &QLineEdit::returnPressed,
                this, [=]() { doGST(); });
        connect(rateCombo,       QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, [=]() { customRateField->clear(); doGST(); });
        connect(modeCombo,       QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, [=]() { doGST(); });

        auto *wrapper = new QWidget(gridContainer);
        auto *wl = new QVBoxLayout(wrapper);
        wl->setContentsMargins(0, 0, 0, 0);
        wl->addWidget(scroll);
        buttonGrid->addWidget(wrapper, 0, 0);
    }

    // ─────────────────────────────────────────
    //  GEOMETRY (Area + Volume)
    // ─────────────────────────────────────────
    void renderGeometry() {
        if (!isMaximizedState) resize(400, 580);

        auto *scroll = new QScrollArea(gridContainer);
        scroll->setWidgetResizable(true);
        scroll->setStyleSheet("QScrollArea{border:none;background:transparent;}");
        auto *container = new QWidget();
        scroll->setWidget(container);
        auto *vl = new QVBoxLayout(container);
        vl->setSpacing(8);
        vl->setContentsMargins(4, 4, 4, 4);

        auto sectionTitle = [&](const QString &t) {
            auto *lbl = new QLabel(t, container);
            lbl->setStyleSheet("font-size:12px;font-weight:bold;color:#FF9F0A;margin-top:6px;");
            vl->addWidget(lbl);
        };

        struct GeoFormula {
            QString name;
            QStringList inputs;
            std::function<double(QList<double>)> calc;
            QString unit;
        };

        QList<GeoFormula> formulas = {
                                      {"Square", {"Side (a)"}, [](QList<double> v){ return CalculatorEngine::instance()->calculateArea("Square", v); }, "units²"},
                                      {"Rectangle", {"Length","Width"}, [](QList<double> v){ return CalculatorEngine::instance()->calculateArea("Rectangle", v); }, "units²"},
                                      {"Triangle", {"Base","Height"}, [](QList<double> v){ return CalculatorEngine::instance()->calculateArea("Triangle", v); }, "units²"},
                                      {"Circle", {"Radius"}, [](QList<double> v){ return CalculatorEngine::instance()->calculateArea("Circle", v); }, "units²"},
                                      {"Parallelogram", {"Base","Height"}, [](QList<double> v){ return CalculatorEngine::instance()->calculateArea("Parallelogram", v); }, "units²"},
                                      {"Trapezoid", {"Base1","Base2","Height"}, [](QList<double> v){ return CalculatorEngine::instance()->calculateArea("Trapezoid", v); }, "units²"},
                                      {"Ellipse", {"Semi-major (a)","Semi-minor (b)"}, [](QList<double> v){ return CalculatorEngine::instance()->calculateArea("Ellipse", v); }, "units²"},
                                      {"Rhombus", {"Diagonal1","Diagonal2"}, [](QList<double> v){ return CalculatorEngine::instance()->calculateArea("Rhombus", v); }, "units²"},
                                      {"Regular Hexagon", {"Side"}, [](QList<double> v){ return CalculatorEngine::instance()->calculateArea("Regular Hexagon", v); }, "units²"},
                                      {"Regular Pentagon", {"Side"}, [](QList<double> v){ return CalculatorEngine::instance()->calculateArea("Regular Pentagon", v); }, "units²"},
                                      {"Sector", {"Radius","Angle (°)"}, [](QList<double> v){ return CalculatorEngine::instance()->calculateArea("Sector", v); }, "units²"},
                                      {"Circle Circumference", {"Radius"}, [](QList<double> v){ return CalculatorEngine::instance()->calculateArea("Circle Circumference", v); }, "units"},
                                      {"Cube", {"Side"}, [](QList<double> v){ return CalculatorEngine::instance()->calculateArea("Cube", v); }, "units³"},
                                      {"Cuboid/Box", {"Length","Width","Height"}, [](QList<double> v){ return CalculatorEngine::instance()->calculateArea("Cuboid/Box", v); }, "units³"},
                                      {"Cylinder", {"Radius","Height"}, [](QList<double> v){ return CalculatorEngine::instance()->calculateArea("Cylinder", v); }, "units³"},
                                      {"Sphere", {"Radius"}, [](QList<double> v){ return CalculatorEngine::instance()->calculateArea("Sphere", v); }, "units³"},
                                      {"Cone", {"Radius","Height"}, [](QList<double> v){ return CalculatorEngine::instance()->calculateArea("Cone", v); }, "units³"},
                                      {"Pyramid (square)", {"Base Side","Height"}, [](QList<double> v){ return CalculatorEngine::instance()->calculateArea("Pyramid (square)", v); }, "units³"},
                                      {"Hemisphere", {"Radius"}, [](QList<double> v){ return CalculatorEngine::instance()->calculateArea("Hemisphere", v); }, "units³"},
                                      {"Torus", {"Major R","Minor r"}, [](QList<double> v){ return CalculatorEngine::instance()->calculateArea("Torus", v); }, "units³"},
                                      {"Prism (triangle)", {"Base","Height (tri)","Length"}, [](QList<double> v){ return CalculatorEngine::instance()->calculateArea("Prism (triangle)", v); }, "units³"},
                                      {"Sphere Surface", {"Radius"}, [](QList<double> v){ return CalculatorEngine::instance()->calculateArea("Sphere Surface", v); }, "units²"},
                                      {"Cylinder Surface", {"Radius","Height"}, [](QList<double> v){ return CalculatorEngine::instance()->calculateArea("Cylinder Surface", v); }, "units²"},
                                      {"Cone Surface", {"Radius","Slant (l)"}, [](QList<double> v){ return CalculatorEngine::instance()->calculateArea("Cone Surface", v); }, "units²"},
                                      };

        bool showedAreaHeader   = false;
        bool showedVolumeHeader = false;

        for (int fi = 0; fi < formulas.size(); fi++) {
            QString  fName   = formulas[fi].name;
            QStringList fInputs = formulas[fi].inputs;
            std::function<double(QList<double>)> fCalc = formulas[fi].calc;
            QString  fUnit   = formulas[fi].unit;

            if (!searchTarget.isEmpty()) {
                QString nameLower = fName.toLower();
                QString target    = searchTarget.toLower();
                bool matches =
                    nameLower.contains(target) ||
                    target.contains(nameLower) ||
                    target.contains(nameLower.split(" ").first().toLower());

                if (target.contains("area")         && fUnit.contains("²") && !fUnit.contains("³")) matches = true;
                if (target.contains("volume")       && fUnit.contains("³"))                          matches = true;
                if (target.contains("surface")      && fName.toLower().contains("surface"))           matches = true;
                if (target.contains("circumference")&& fName.toLower().contains("circumference"))    matches = true;
                if (target.contains("perimeter")    && fName.toLower().contains("circumference"))    matches = true;

                if (!matches) continue;
            }

            if (!showedAreaHeader && fUnit.contains("²") && !fUnit.contains("³") && !fName.contains("Surface")) {
                sectionTitle("▶  2D Area  &  Perimeter");
                showedAreaHeader = true;
            }
            if (!showedVolumeHeader && (fUnit.contains("³") || fName.contains("Surface"))) {
                if (fi >= 12) {
                    sectionTitle("▶  3D Volume  &  Surface Area");
                    showedVolumeHeader = true;
                }
            }

            auto *card = new QWidget(container);
            card->setObjectName("geoCard");
            auto *cl = new QVBoxLayout(card);
            cl->setContentsMargins(8, 6, 8, 6);
            cl->setSpacing(4);

            auto *nameLbl = new QLabel(fName, card);
            nameLbl->setStyleSheet("font-weight:bold;font-size:11px;");
            cl->addWidget(nameLbl);

            QList<QLineEdit*> fields;
            for (const QString &inp : fInputs) {
                auto *rowW = new QHBoxLayout();
                auto *lbl  = new QLabel(inp + ":", card);
                lbl->setFixedWidth(100);
                lbl->setStyleSheet("font-size:10px;");
                auto *le = new QLineEdit("0", card);
                le->setStyleSheet(
                    "height:24px;background:#2C2C2E;color:white;"
                    "border-radius:4px;padding-left:4px;font-size:11px;");
                rowW->addWidget(lbl);
                rowW->addWidget(le);
                cl->addLayout(rowW);
                fields << le;
            }

            auto *resultLbl = new QLabel("= 0  " + fUnit, card);
            resultLbl->setStyleSheet("color:#34C759;font-size:12px;font-weight:bold;padding-top:2px;");
            cl->addWidget(resultLbl);

            auto doCalc = [=]() {
                QList<double> vals;
                for (auto *le : fields) vals << le->text().toDouble();
                double res = fCalc(vals);
                QString formattedRes = CalculatorEngine::instance()->formatResult(res);
                resultLbl->setText(QString("= %1  %2").arg(formattedRes).arg(fUnit));
                display->setText(formattedRes);
                HistoryManager::instance()->addToHistory(QString("%1: %2 %3").arg(fName).arg(formattedRes).arg(fUnit));
            };

            auto *calcBtn = new QPushButton("Calculate", card);
            calcBtn->setObjectName("actionButton");
            calcBtn->setFixedHeight(26);
            connect(calcBtn, &QPushButton::clicked, this, [=]() { doCalc(); });
            for (auto *le : fields)
                connect(le, &QLineEdit::returnPressed, this, [=]() { doCalc(); });
            cl->addWidget(calcBtn);

            card->setStyleSheet("#geoCard{background:rgba(60,60,62,180);border-radius:8px;}");
            vl->addWidget(card);
        }

        vl->addStretch();
        auto *wrapper = new QWidget(gridContainer);
        auto *wl = new QVBoxLayout(wrapper);
        wl->setContentsMargins(0, 0, 0, 0);
        wl->addWidget(scroll);
        buttonGrid->addWidget(wrapper, 0, 0);
    }

    // ─────────────────────────────────────────
    //  UNIT CONVERTER
    // ─────────────────────────────────────────
    void renderUnitConverter() {
        if (!isMaximizedState) resize(400, 500);

        QList<UnitGroup> groups = CalculatorEngine::instance()->getUnitGroups();

        auto *scroll = new QScrollArea(gridContainer);
        scroll->setWidgetResizable(true);
        scroll->setStyleSheet("QScrollArea{border:none;background:transparent;}");
        auto *cont = new QWidget();
        scroll->setWidget(cont);
        auto *vl = new QVBoxLayout(cont);
        vl->setSpacing(8);
        vl->setContentsMargins(4, 4, 4, 4);

        for (const auto &grp : groups) {
            if (!searchTarget.isEmpty()) {
                QString target   = searchTarget.toLower();
                QString grpLower = grp.name.toLower();
                bool matches = grpLower.contains(target) || target.contains(grpLower);

                for (const QString &unit : grp.units) {
                    if (unit.toLower().contains(target)) { matches = true; break; }
                }
                if (target.contains("inch") || target.contains("feet") || target.contains("foot") ||
                    target.contains("km")   || target.contains("mile") ||
                    target.contains("meter")|| target.contains("metre"))
                    if (grp.name == "Length") matches = true;

                if (target.contains("kg") || target.contains("gram") ||
                    target.contains("pound") || target.contains("ounce"))
                    if (grp.name == "Mass / Weight") matches = true;

                if (target.contains("celsius") || target.contains("fahrenheit") ||
                    target.contains("kelvin")   || target.contains("temperature"))
                    if (grp.name == "Temperature") matches = true;

                if (!matches) continue;
            }

            auto *card = new QWidget(cont);
            card->setObjectName("geoCard");
            auto *cl = new QVBoxLayout(card);
            cl->setContentsMargins(8, 6, 8, 6);
            cl->setSpacing(4);

            auto *hdr = new QLabel("⟳  " + grp.name, card);
            hdr->setStyleSheet("font-weight:bold;font-size:11px;color:#FF9F0A;");
            cl->addWidget(hdr);

            auto *valIn = new QLineEdit("1", card);
            valIn->setStyleSheet(
                "height:26px;background:#2C2C2E;color:white;"
                "border-radius:4px;padding-left:6px;font-size:12px;");

            auto *fromU = new QComboBox(card);
            auto *toU   = new QComboBox(card);
            fromU->addItems(grp.units);
            toU->addItems(grp.units);
            if (grp.units.size() > 1) toU->setCurrentIndex(1);
            for (auto *cb : {fromU, toU})
                cb->setStyleSheet(
                    "height:28px;background:#3A3A3C;color:white;"
                    "padding-left:4px;border-radius:4px;");

            auto *resLbl = new QLabel("= 0", card);
            resLbl->setStyleSheet("color:#34C759;font-size:13px;font-weight:bold;");

            auto *rowW = new QHBoxLayout();
            rowW->addWidget(fromU);

            auto *swapBtn = new QPushButton("⇄", card);
            swapBtn->setStyleSheet("font-size:16px;color:#FF9F0A;font-weight:bold;background:transparent;border:none;max-width:30px;");
            swapBtn->setCursor(Qt::PointingHandCursor);
            swapBtn->setFocusPolicy(Qt::NoFocus);
            connect(swapBtn, &QPushButton::clicked, this, [=]() {
                int fi = fromU->currentIndex();
                int ti = toU->currentIndex();
                fromU->setCurrentIndex(ti);
                toU->setCurrentIndex(fi);
            });
            rowW->addWidget(swapBtn);

            rowW->addWidget(toU);
            cl->addWidget(valIn);
            cl->addLayout(rowW);
            cl->addWidget(resLbl);

            UnitGroup grpCopy = grp;
            std::function<void()> doConvert = [=]() mutable {
                double val = valIn->text().toDouble();
                int fi = fromU->currentIndex();
                int ti = toU->currentIndex();
                double res = CalculatorEngine::instance()->convertUnit(grpCopy.name, val, fi, ti);
                QString formattedRes = CalculatorEngine::instance()->formatResult(res);
                resLbl->setText(
                    QString("= %1 %2").arg(formattedRes).arg(grpCopy.units[ti]));
                display->setText(formattedRes);
            };

            connect(valIn, &QLineEdit::textChanged,
                    this, [doConvert](const QString &) { doConvert(); });
            connect(fromU, QOverload<int>::of(&QComboBox::currentIndexChanged),
                    this, [doConvert](int) { doConvert(); });
            connect(toU,   QOverload<int>::of(&QComboBox::currentIndexChanged),
                    this, [doConvert](int) { doConvert(); });
            doConvert();

            card->setStyleSheet("#geoCard{background:rgba(60,60,62,180);border-radius:8px;}");
            vl->addWidget(card);
        }

        vl->addStretch();
        auto *wrapper = new QWidget(gridContainer);
        auto *wl = new QVBoxLayout(wrapper);
        wl->setContentsMargins(0, 0, 0, 0);
        wl->addWidget(scroll);
        buttonGrid->addWidget(wrapper, 0, 0);
    }

    // ─────────────────────────────────────────
    //  BUTTON CLICK
    // ─────────────────────────────────────────
    void handleButtonClick() {
        QPushButton *clickedButton = qobject_cast<QPushButton*>(sender());
        if (!clickedButton) return;
        processInputToken(clickedButton->text());
    }

    // ─────────────────────────────────────────
    //  CORE CALCULATION ENGINE
    // ─────────────────────────────────────────
    void processInputToken(QString token) {
        QString calcText = display->text();
        calcText.remove(",").remove(" ");
        double val = calcText.toDouble();

        if ((token >= "0" && token <= "9") || token == "00" || token == "000" || token == ".") {
            if (token == "." && display->text().contains(".")) return;
            if (display->text() == "0" || start_new_num) {
                display->setText((token == "00" || token == "000") ? "0" : token);
                start_new_num = false;
            } else {
                display->setText(display->text() + token);
            }
            return;
        }

        if (token == "AC" || token == "C" || token == "CE") {
            display->setText("0");
            current_val = 0; last_op = ""; start_new_num = true;
            expressionLabel->clear(); expressionMode = false;
            return;
        }
        if (token == "DEL") {
            QString cur = display->text();
            if (cur.length() > 1) cur.chop(1);
            else { cur = "0"; start_new_num = true; }
            display->setText(cur);
            return;
        }

        auto setDisplay = [&](double v) {
            display->setText(CalculatorEngine::instance()->formatResult(v));
            start_new_num = true;
        };
        auto toAngle   = [&](double d) -> double { return isDegMode ? d * M_PI / 180.0 : d; };
        auto fromAngle = [&](double r) -> double { return isDegMode ? r * 180.0 / M_PI : r; };

        if (token == "+/-")               { setDisplay(-val); return; }
        if (token == "%")                 { setDisplay(val / 100.0); return; }
        if (token == "1/x")              { setDisplay(val != 0 ? 1.0 / val : NAN); return; }
        if (token == "x^2")              { setDisplay(val * val); return; }
        if (token == "x^3")              { setDisplay(val * val * val); return; }
        if (token == "sqrt" || token == "√") { setDisplay(val >= 0 ? std::sqrt(val) : NAN); return; }
        if (token == "∛")                { setDisplay(std::cbrt(val)); return; }
        if (token == "abs")              { setDisplay(std::fabs(val)); return; }

        if (token == "x!") {
            long long n = (long long)std::round(val);
            double f = 1;
            for (long long i = 2; i <= n && i <= 20; i++) f *= i;
            setDisplay(f); return;
        }
        if (token == "ln")    { setDisplay(val > 0 ? std::log(val)   : NAN); return; }
        if (token == "log")   { setDisplay(val > 0 ? std::log10(val) : NAN); return; }
        if (token == "e^x")   { setDisplay(std::exp(val)); return; }
        if (token == "10^x")  { setDisplay(std::pow(10.0, val)); return; }
        if (token == "sin")   { setDisplay(std::sin(toAngle(val))); return; }
        if (token == "cos")   { setDisplay(std::cos(toAngle(val))); return; }
        if (token == "tan")   { setDisplay(std::tan(toAngle(val))); return; }
        if (token == "asin")  { setDisplay(fromAngle(std::asin(val))); return; }
        if (token == "acos")  { setDisplay(fromAngle(std::acos(val))); return; }
        if (token == "atan")  { setDisplay(fromAngle(std::atan(val))); return; }
        if (token == "sinh")  { setDisplay(std::sinh(val)); return; }
        if (token == "cosh")  { setDisplay(std::cosh(val)); return; }
        if (token == "tanh")  { setDisplay(std::tanh(val)); return; }
        if (token == "asinh") { setDisplay(std::asinh(val)); return; }
        if (token == "acosh") { setDisplay(val >= 1 ? std::acosh(val) : NAN); return; }
        if (token == "atanh") { setDisplay(std::fabs(val) < 1 ? std::atanh(val) : NAN); return; }
        if (token == "pi")    { display->setText(QString::number(M_PI, 'g', 10)); start_new_num = false; return; }
        if (token == "e")     { display->setText(QString::number(M_E,  'g', 10)); start_new_num = false; return; }
        if (token == "EE")    { display->setText(QString::number(val) + "e+"); start_new_num = false; return; }
        if (token == "Rand")  { setDisplay((double)rand() / RAND_MAX); return; }

        auto compute = [&](double a, const QString &op, double b) -> double {
            return CalculatorEngine::instance()->calculateOperation(a, op, b);
        };

        if (token == "+" || token == "-"   || token == "x"   || token == "/" ||
            token == "x^y" || token == "y√x" || token == "|mod|") {

            if (expressionMode) {
                QString expr = display->text();
                expr.remove(",").remove(" ");
                QRegularExpression re("^(-?[\\d.]+)([+\\-x/])(-?[\\d.]+)$");
                auto m = re.match(expr);
                if (m.hasMatch()) {
                    double a = m.captured(1).toDouble(), b = m.captured(3).toDouble();
                    current_val  = compute(a, m.captured(2), b);
                    display->setText(CalculatorEngine::instance()->formatResult(current_val));
                    expressionMode = false;
                }
            } else {
                if (!last_op.isEmpty() && !start_new_num) {
                    current_val = compute(current_val, last_op, val);
                    display->setText(CalculatorEngine::instance()->formatResult(current_val));
                } else {
                    current_val = val;
                }
            }
            last_op = token;
            start_new_num = true;
            expressionLabel->setText(CalculatorEngine::instance()->formatResult(current_val) + "  " + token);
            return;
        }

        if (token == "=") {
            if (last_op.isEmpty()) return;

            double operand;
            if (start_new_num) {
                operand = last_val;
            } else {
                operand  = val;
                last_val = val;
            }

            double result = compute(current_val, last_op, operand);

            expressionLabel->setText(
                QString("%1  %2  %3  =")
                    .arg(CalculatorEngine::instance()->formatResult(current_val))
                    .arg(last_op)
                    .arg(CalculatorEngine::instance()->formatResult(operand)));

            HistoryManager::instance()->addToHistory(
                QString("%1 %2 %3 = %4")
                    .arg(CalculatorEngine::instance()->formatResult(current_val))
                    .arg(last_op)
                    .arg(CalculatorEngine::instance()->formatResult(operand))
                    .arg(CalculatorEngine::instance()->formatResult(result)));

            display->setText(CalculatorEngine::instance()->formatResult(result));
            current_val = result;
            start_new_num = true;
            return;
        }

        if (token == "(") { expressionLabel->setText("("); start_new_num = true; return; }
        if (token == ")") { return; }
    }

    // ─────────────────────────────────────────
    //  KEY PRESS
    // ─────────────────────────────────────────
    void keyPressEvent(QKeyEvent *event) override {
        // ── Menu ESC handling first ──
        if (event->key() == Qt::Key_Escape) {
            if (themesSubmenu) { toggleThemesSubmenu(); return; }
            if (toolsPopup)    { closeToolsMenu();      return; }
            if (aboutPopup)    {
                aboutPopup->close(); aboutPopup->deleteLater(); aboutPopup = nullptr;
                aboutEmailLabel = nullptr; aboutPhotoLabel = nullptr; aboutLoginBtn = nullptr;
                return;
            }
        }

        // ── Ctrl+F dispatch: history-search when panel is open & focused ──
        if (event->modifiers() == Qt::ControlModifier && event->key() == Qt::Key_F) {
            if (is_history_visible &&
                (historyList->hasFocus() || historyWidget->isActiveWindow() ||
                 (historySearchBox && historySearchBox->hasFocus()))) {
                toggleHistorySearch();
            } else {
                toggleSearchBar();
            }
            return;
        }

        if (historySearchBox && historySearchBox->hasFocus() && event->key() == Qt::Key_Escape) {
            toggleHistorySearch();
            return;
        }

        // Let search bar handle its own keys
        if (quickSearch && quickSearch->isVisible() && quickSearch->hasFocus()) {
            if (event->key() == Qt::Key_Escape) {
                quickSearch->hide();
                searchTarget.clear();
                return;
            }
            QMainWindow::keyPressEvent(event);
            return;
        }

        if (current_view_mode == MODE_CURRENCY ||
            current_view_mode == MODE_GEOMETRY ||
            current_view_mode == MODE_UNIT     ||
            current_view_mode == MODE_EMI      ||
            current_view_mode == MODE_GST) {
            QMainWindow::keyPressEvent(event);
            return;
        }

        if (event->matches(QKeySequence::Paste)) { display->paste(); return; }
        if (event->matches(QKeySequence::Copy))  {
            QApplication::clipboard()->setText(display->text()); return;
        }

        int     key  = event->key();
        QString text = event->text();

        if      (key >= Qt::Key_0 && key <= Qt::Key_9) processInputToken(text);
        else if (key == Qt::Key_Period || text == ".")  processInputToken(".");
        else if (key == Qt::Key_Plus)                  processInputToken("+");
        else if (key == Qt::Key_Minus)                 processInputToken("-");
        else if (key == Qt::Key_Asterisk || text == "x" || text == "X") processInputToken("x");
        else if (key == Qt::Key_Slash)                 processInputToken("/");
        else if (key == Qt::Key_Percent)               processInputToken("%");
        else if (key == Qt::Key_Enter  ||
                 key == Qt::Key_Return ||
                 key == Qt::Key_Equal)                 processInputToken("=");
        else if (key == Qt::Key_Backspace)             processInputToken("DEL");
        else if (key == Qt::Key_Delete ||
                 key == Qt::Key_Escape)                processInputToken("C");
        else if (key == Qt::Key_End) {
            display->setText("0"); start_new_num = true;
        }
    }

    // ─────────────────────────────────────────
    //  STYLESHEET
    // ─────────────────────────────────────────
    void Apply1OSStyleSheet() {
        this->setStyleSheet(ThemeService::instance()->getStyleSheet());
    }

protected:
    QPoint dragPosition;

    // ─────────────────────────────────────────
    //  EVENT FILTER
    // ─────────────────────────────────────────
    bool eventFilter(QObject *obj, QEvent *event) override {

        // ── Menus Click Propagation ──
        if (event->type() == QEvent::MouseButtonPress && (obj == toolsPopup || obj == themesSubmenu)) {
            // let it propagate to the buttons
            return false;
        }

        if (event->type() == QEvent::MouseButtonPress && (toolsPopup || themesSubmenu || aboutPopup)) {
            QWidget *w = qobject_cast<QWidget*>(obj);
            bool insideMenu =
                (toolsPopup    && (w == toolsPopup    || (w && toolsPopup->isAncestorOf(w)))) ||
                (themesSubmenu && (w == themesSubmenu || (w && themesSubmenu->isAncestorOf(w)))) ||
                (aboutPopup    && (w == aboutPopup    || (w && aboutPopup->isAncestorOf(w))));

            if (!insideMenu) {
                closeToolsMenu();
                if (aboutPopup && (!w || !aboutPopup->isAncestorOf(w))) {
                    aboutPopup->close(); aboutPopup->deleteLater(); aboutPopup = nullptr;
                    aboutEmailLabel = nullptr; aboutPhotoLabel = nullptr; aboutLoginBtn = nullptr;
                }
            }
        }

        // ── Click outside search bar → hide it ──
        if (event->type() == QEvent::MouseButtonPress) {
            if (quickSearch && quickSearch->isVisible()) {
                QWidget *w = qobject_cast<QWidget*>(obj);
                bool clickedInsideSearch = (w && (w == quickSearch ||
                                                  (quickSearch->completer() && quickSearch->completer()->popup() &&
                                                   w == quickSearch->completer()->popup())));
                if (!clickedInsideSearch) {
                    quickSearch->hide();
                    searchTarget.clear();
                }
            }
        }

        // ── Search bar ESC ────────────────────
        if (obj == quickSearch && event->type() == QEvent::KeyPress) {
            QKeyEvent *ke = static_cast<QKeyEvent*>(event);
            if (ke->key() == Qt::Key_Escape) {
                quickSearch->hide();
                searchTarget.clear();
                return true;
            }
        }

        // ── Display keyboard passthrough ──────
        if (obj == display && event->type() == QEvent::KeyPress) {
            QKeyEvent *ke = static_cast<QKeyEvent*>(event);
            if (ke->key() == Qt::Key_Backspace) { processInputToken("DEL"); return true; }
            if (ke->key() == Qt::Key_Delete)    { processInputToken("C");   return true; }
            if (ke->key() == Qt::Key_End)       { display->setText("0"); start_new_num = true; return true; }
        }

        // ── Traffic-light hover icons ─────────
        if (event->type() == QEvent::Enter) {
            if (obj == btnClose) { btnClose->setIcon(QIcon("END.svg"));       btnClose->setIconSize(QSize(9,9));  }
            if (obj == btnMin)   { btnMin->setIcon(QIcon("Drop Down.svg"));   btnMin->setIconSize(QSize(11,11));  }
            if (obj == btnMax)   { btnMax->setIcon(QIcon(isMaximizedState ? "Default.svg" : "Full View.svg"));
                btnMax->setIconSize(QSize(11,11)); }
        }

        if (event->type() == QEvent::Leave) {
            if (obj == btnClose) btnClose->setIcon(QIcon());
            if (obj == btnMin)   btnMin->setIcon(QIcon());
            if (obj == btnMax)   btnMax->setIcon(QIcon());
        }

        // ── Click on expressionLabel → reload expression ──
        if (obj == expressionLabel && event->type() == QEvent::MouseButtonPress) {
            QString expr = expressionLabel->text();
            if (!expr.isEmpty()) {
                expr.remove("=");
                display->setText(expr);
                expressionMode = true;
                start_new_num = false;
            }
            return true;
        }

        return QMainWindow::eventFilter(obj, event);
    }

    // ─────────────────────────────────────────
    //  DRAG (only from top bar, y < 42)
    // ─────────────────────────────────────────
    void mousePressEvent(QMouseEvent *event) override {
        if (event->button() == Qt::LeftButton && event->position().y() < 42)
            dragPosition = event->globalPosition().toPoint() - frameGeometry().topLeft();
        QMainWindow::mousePressEvent(event);
    }

    void mouseMoveEvent(QMouseEvent *event) override {
        if ((event->buttons() & Qt::LeftButton) && !dragPosition.isNull())
            move(event->globalPosition().toPoint() - dragPosition);
        QMainWindow::mouseMoveEvent(event);
    }

    void mouseReleaseEvent(QMouseEvent *event) override {
        dragPosition = QPoint();
        QMainWindow::mouseReleaseEvent(event);
    }
};

// ─────────────────────────────────────────────
//  MAIN
// ─────────────────────────────────────────────
int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("1OS Calculator");
    app.setOrganizationName("1OS");

    qDebug() << "Locale:" << QLocale::system().name();

    QSettings reg(
        "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
        QSettings::NativeFormat);
    bool light = (reg.value("AppsUseLightTheme", 0).toInt() == 1);
    app.setWindowIcon(QIcon(light ? "CalculatorLight.ico" : "CalculatorDark.ico"));

    OS1Calculator calc;
    calc.show();
    return app.exec();
}