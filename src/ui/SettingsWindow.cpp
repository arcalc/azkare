#include "SettingsWindow.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QDialogButtonBox>
#include <QScrollArea>
#include <QColorDialog>
#include <QDesktopServices>
#include <QUrl>
#include <QApplication>
#include <QScreen>
#include <QDebug>

#include "../core/AppSettings.h"
#include "../core/AdhkarDatabase.h"
#include "../core/Scheduler.h"
#include "../core/Updater.h"
#include "../overlay/OverlayManager.h"
#include "../notifications/NotificationDispatcher.h"
#include "../platform/AutostartManager.h"
#include "../platform/TrayController.h"

// ── Constructor ───────────────────────────────────────────────────────────────

SettingsWindow::SettingsWindow(AppSettings            &settings,
                               AdhkarDatabase         &db,
                               OverlayManager         &overlay,
                               NotificationDispatcher &notifier,
                               Scheduler              &scheduler,
                               TrayController         &tray,
                               QWidget                *parent)
    : QMainWindow(parent)
    , m_settings(settings)
    , m_db(db)
    , m_overlay(overlay)
    , m_notifier(notifier)
    , m_scheduler(scheduler)
    , m_tray(tray)
{
    setWindowTitle("أذكاري — الإعدادات");
    setMinimumSize(700, 540);
    resize(760, 580);

    // Central widget
    QWidget *central = new QWidget(this);
    setCentralWidget(central);

    QVBoxLayout *mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Tab widget
    m_tabs = new QTabWidget(central);
    m_tabs->addTab(buildGeneralTab(),       "عام");
    m_tabs->addTab(buildDisplayTab(),       "طريقة العرض");
    m_tabs->addTab(buildAppearanceTab(),    "المظهر");
    m_tabs->addTab(buildNotificationsTab(), "الإشعارات");
    m_tabs->addTab(buildContentTab(),       "المحتوى");
    m_tabs->addTab(buildAboutTab(),         "حول التطبيق");
    mainLayout->addWidget(m_tabs);

    // Bottom button bar
    QWidget     *btnBar    = new QWidget(central);
    QHBoxLayout *btnLayout = new QHBoxLayout(btnBar);
    btnLayout->setContentsMargins(16, 12, 16, 12);

    QPushButton *btnReset  = new QPushButton("استعادة الافتراضي");
    QPushButton *btnCancel = new QPushButton("إلغاء");
    QPushButton *btnApply  = new QPushButton("تطبيق");
    QPushButton *btnSave   = new QPushButton("حفظ");
    btnSave->setDefault(true);

    btnLayout->addWidget(btnReset);
    btnLayout->addStretch();
    btnLayout->addWidget(btnCancel);
    btnLayout->addWidget(btnApply);
    btnLayout->addWidget(btnSave);
    mainLayout->addWidget(btnBar);

    connect(btnSave,   &QPushButton::clicked, this, &SettingsWindow::onSave);
    connect(btnCancel, &QPushButton::clicked, this, &SettingsWindow::onCancel);
    connect(btnApply,  &QPushButton::clicked, this, &SettingsWindow::onApply);
    connect(btnReset,  &QPushButton::clicked, this, [this]() {
        // Reset to compiled defaults — just reconstruct an AppSettings
        AppSettings fresh;
        m_settings = fresh;
        loadToUi();
    });

    loadToUi();
}

void SettingsWindow::showAndRaise()
{
    loadToUi();
    show();
    raise();
    activateWindow();
}

// ── Tab builders ──────────────────────────────────────────────────────────────

/**
 * Helper: adds a labeled control row with a plain-language helper subtitle.
 * Layout:
 *   [Label]
 *   [Control widget]
 *   [Helper subtitle in muted smaller text]
 */
void SettingsWindow::addFormRow(QWidget       *container,
                                QLayout       *layout,
                                const QString &label,
                                QWidget       *control,
                                const QString &helpText)
{
    QVBoxLayout *vl = qobject_cast<QVBoxLayout *>(layout);
    if (!vl) return;

    QLabel *lbl = new QLabel(label, container);
    QFont f = lbl->font();
    f.setBold(true);
    lbl->setFont(f);
    vl->addWidget(lbl);

    vl->addWidget(control);

    if (!helpText.isEmpty()) {
        QLabel *help = new QLabel(helpText, container);
        help->setWordWrap(true);
        QPalette p = help->palette();
        p.setColor(QPalette::WindowText, p.color(QPalette::Mid));
        help->setPalette(p);
        QFont hf = help->font();
        hf.setPointSizeF(hf.pointSizeF() * 0.88);
        help->setFont(hf);
        vl->addWidget(help);
    }

    // Spacer between rows
    vl->addSpacing(12);
}

QWidget *SettingsWindow::buildGeneralTab()
{
    QScrollArea *scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);

    QWidget     *w  = new QWidget;
    QVBoxLayout *vl = new QVBoxLayout(w);
    vl->setContentsMargins(24, 24, 24, 24);
    vl->setSpacing(0);

    // Language
    m_langCombo = new QComboBox;
    m_langCombo->addItem("العربية", "ar");
    m_langCombo->addItem("English", "en");
    connect(m_langCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SettingsWindow::onLanguageChanged);

    addFormRow(w, vl, "اللغة",
               m_langCombo,
               "اختر لغة واجهة التطبيق. سيتغير التخطيط تلقائيًا بين اليمين لليسار (عربي) واليسار لليمين (إنجليزي).");

    // Autostart
    m_autostartChk = new QCheckBox("تفعيل التشغيل التلقائي عند بدء النظام");
    addFormRow(w, vl, "التشغيل عند بدء الجهاز",
               m_autostartChk,
               "عند تفعيل هذا الخيار، سيبدأ تطبيق أذكاري تلقائيًا عند كل مرة تشغّل فيها جهازك دون الحاجة لفتحه يدويًا.");

    vl->addStretch();
    scroll->setWidget(w);
    return scroll;
}

QWidget *SettingsWindow::buildDisplayTab()
{
    QScrollArea *scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);

    QWidget     *w  = new QWidget;
    QVBoxLayout *vl = new QVBoxLayout(w);
    vl->setContentsMargins(24, 24, 24, 24);
    vl->setSpacing(0);

    // Display mode
    m_modeCombo = new QComboBox;
    m_modeCombo->addItem("على خلفية سطح المكتب فقط",  int(DisplayMode::OverlayOnly));
    m_modeCombo->addItem("إشعارات فقط",               int(DisplayMode::NotificationsOnly));
    m_modeCombo->addItem("الاثنين معاً",               int(DisplayMode::Both));
    connect(m_modeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SettingsWindow::onDisplayModeChanged);
    addFormRow(w, vl, "وضع العرض", m_modeCombo,
               "اختر كيف تريد رؤية الأذكار: هل تريدها على خلفية سطح المكتب، أم كإشعارات فقط، أم كليهما معاً؟");

    // Backend (Linux only)
#if defined(Q_OS_LINUX)
    m_backendCombo = new QComboBox;
    m_backendCombo->addItem("المحرك الافتراضي (موصى به)", int(OverlayBackend::Native));
    m_backendCombo->addItem("eww (لينكس — متقدم)",        int(OverlayBackend::Eww));
    connect(m_backendCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SettingsWindow::onBackendChanged);
    addFormRow(w, vl, "محرك العرض", m_backendCombo,
               "الطريقة التقنية التي يستخدمها التطبيق لعرض النص على سطح مكتبك. المحرك الافتراضي يعمل على جميع الأنظمة بدون أي إعداد إضافي. أما eww فهو لمستخدمي لينكس الذين يستخدمونه بالفعل.");
#endif

    // Position
    m_posCombo = new QComboBox;
    m_posCombo->addItem("أعلى اليسار",   int(OverlayPosition::TopLeft));
    m_posCombo->addItem("أعلى الوسط",   int(OverlayPosition::TopCenter));
    m_posCombo->addItem("أعلى اليمين",  int(OverlayPosition::TopRight));
    m_posCombo->addItem("وسط الشاشة",   int(OverlayPosition::Center));
    m_posCombo->addItem("أسفل اليسار",  int(OverlayPosition::BottomLeft));
    m_posCombo->addItem("أسفل الوسط",   int(OverlayPosition::BottomCenter));
    m_posCombo->addItem("أسفل اليمين",  int(OverlayPosition::BottomRight));
    m_posCombo->addItem("موضع مخصص",    int(OverlayPosition::Custom));
    connect(m_posCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SettingsWindow::onPositionChanged);
    addFormRow(w, vl, "موضع النص على الشاشة", m_posCombo,
               "حدد في أي مكان من الشاشة تريد ظهور نص الذكر. اختر موضعًا محددًا مسبقًا، أو اضغط 'تفعيل وضع السحب' لتحريك النص بالماوس.");

    // Drag mode
    m_dragModeChk = new QCheckBox("تفعيل وضع السحب (اسحب النص بالماوس إلى أي مكان)");
    connect(m_dragModeChk, &QCheckBox::toggled, this, &SettingsWindow::onDragModeToggled);
    addFormRow(w, vl, "السحب والإفلات", m_dragModeChk,
               "عند التفعيل، يمكنك النقر على نص الذكر وسحبه إلى أي مكان تريده على الشاشة. يتذكر التطبيق الموضع الجديد.");

    // Monitor selector
    m_monitorCombo = new QComboBox;
    const auto screens = QApplication::screens();
    for (int i = 0; i < screens.size(); ++i) {
        m_monitorCombo->addItem(
            QString("الشاشة %1 (%2×%3)")
                .arg(i + 1)
                .arg(screens[i]->geometry().width())
                .arg(screens[i]->geometry().height()),
            i);
    }
    addFormRow(w, vl, "الشاشة", m_monitorCombo,
               "إذا كان لديك أكثر من شاشة واحدة موصولة بجهازك، اختر على أيها تريد ظهور الأذكار.");

    // Rotation interval
    m_rotationSpin = new QSpinBox;
    m_rotationSpin->setRange(10, 86400);
    m_rotationSpin->setSuffix("  ثانية");
    m_rotationSpin->setSpecialValueText("مخصص");
    addFormRow(w, vl, "مدة عرض كل ذكر", m_rotationSpin,
               "كم من الوقت يظل كل ذكر معروضًا قبل الانتقال إلى التالي؟ على سبيل المثال: ٣٠ ثانية، أو ٣٠٠ ثانية (٥ دقائق).");

    vl->addStretch();
    scroll->setWidget(w);
    return scroll;
}

QWidget *SettingsWindow::buildAppearanceTab()
{
    QScrollArea *scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);

    QWidget     *w  = new QWidget;
    QVBoxLayout *vl = new QVBoxLayout(w);
    vl->setContentsMargins(24, 24, 24, 24);
    vl->setSpacing(0);

    // Font family
    m_fontCombo = new QFontComboBox;
    m_fontCombo->setFontFilters(QFontComboBox::AllFonts);
    connect(m_fontCombo, &QFontComboBox::currentFontChanged,
            this, &SettingsWindow::onFontChanged);
    addFormRow(w, vl, "الخط", m_fontCombo,
               "اختر الخط المستخدم لعرض الأذكار. الخطوط العربية كأميري وشهرازاد مدمجة في التطبيق ومتاحة مباشرة.");

    // Font size
    m_fontSizeSpin = new QSpinBox;
    m_fontSizeSpin->setRange(8, 72);
    m_fontSizeSpin->setSuffix("  نقطة");
    addFormRow(w, vl, "حجم الخط", m_fontSizeSpin,
               "حجم النص المعروض بالنقاط. زد الحجم لقراءة أيسر من بعيد، أو قلله إذا أردت مساحة أصغر على الشاشة.");

    // Text color
    m_colorBtn = new QToolButton;
    m_colorBtn->setText("   اختر اللون   ");
    m_colorBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    connect(m_colorBtn, &QToolButton::clicked, this, &SettingsWindow::onColorPickerClicked);
    addFormRow(w, vl, "لون النص", m_colorBtn,
               "اختر لون الكتابة. إذا كانت خلفية سطح مكتبك فاتحة، اختر لونًا داكنًا، والعكس بالعكس. الأبيض يناسب معظم الخلفيات.");

    // Shadow
    m_shadowChk = new QCheckBox("إضافة ظل للنص");
    addFormRow(w, vl, "ظل النص", m_shadowChk,
               "يضيف هذا الخيار ظلًا خفيفًا حول الحروف يجعل النص أوضح وأسهل قراءة بغض النظر عن لون أو صورة الخلفية. يُنصح بتفعيله دائمًا.");

    // Alignment
    m_alignCombo = new QComboBox;
    m_alignCombo->addItem("يمين",  int(Qt::AlignRight));
    m_alignCombo->addItem("وسط",   int(Qt::AlignCenter));
    m_alignCombo->addItem("يسار",  int(Qt::AlignLeft));
    addFormRow(w, vl, "محاذاة النص", m_alignCombo,
               "كيف تريد محاذاة نص الذكر؟ 'وسط' هو الأنسب لمعظم المواضع. 'يمين' هو الأنسب للنصوص العربية الطويلة.");

    vl->addStretch();
    scroll->setWidget(w);
    return scroll;
}

QWidget *SettingsWindow::buildNotificationsTab()
{
    QScrollArea *scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);

    QWidget     *w  = new QWidget;
    QVBoxLayout *vl = new QVBoxLayout(w);
    vl->setContentsMargins(24, 24, 24, 24);
    vl->setSpacing(0);

    m_notifEnabledChk = new QCheckBox("تفعيل إشعارات الذكر");
    addFormRow(w, vl, "الإشعارات", m_notifEnabledChk,
               "عند التفعيل، سيظهر إشعار على شاشتك كل فترة زمنية تحددها يذكّرك بذكر أو دعاء جديد. تعمل هذه الإشعارات حتى عندما تكون نوافذ التطبيقات مفتوحة.");

    m_notifIntervalSpin = new QSpinBox;
    m_notifIntervalSpin->setRange(60, 86400);
    m_notifIntervalSpin->setSuffix("  ثانية");
    addFormRow(w, vl, "الفترة بين الإشعارات", m_notifIntervalSpin,
               "كم من الوقت بين كل إشعار والآخر؟ ١٨٠٠ ثانية = ٣٠ دقيقة. ٣٦٠٠ ثانية = ساعة واحدة. اختر ما يناسب جدولك اليومي.");

    vl->addStretch();
    scroll->setWidget(w);
    return scroll;
}

QWidget *SettingsWindow::buildContentTab()
{
    QScrollArea *scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);

    QWidget     *w  = new QWidget;
    QVBoxLayout *vl = new QVBoxLayout(w);
    vl->setContentsMargins(24, 24, 24, 24);
    vl->setSpacing(0);

    // Categories group
    QGroupBox   *catBox = new QGroupBox("الفئات المعروضة");
    QVBoxLayout *catVl  = new QVBoxLayout(catBox);

    struct Category { QString id; QString label; };
    const QList<Category> cats = {
        {"morning",     "أذكار الصباح"},
        {"evening",     "أذكار المساء"},
        {"sleep",       "أذكار النوم والاستيقاظ"},
        {"post_prayer", "أذكار بعد الصلاة"},
        {"istighfar",   "الاستغفار والتوبة"},
        {"general",     "أدعية عامة"},
        {"quran",       "أدعية قرآنية"},
    };

    m_categoryChks.clear();
    for (const auto &cat : cats) {
        QCheckBox *chk = new QCheckBox(cat.label);
        chk->setObjectName(cat.id);
        catVl->addWidget(chk);
        m_categoryChks.append(chk);
    }

    vl->addWidget(catBox);

    QLabel *catHelp = new QLabel(
        "اختر أنواع الأذكار والأدعية التي تريد عرضها. يمكنك تفعيل أو إيقاف أي فئة في أي وقت. "
        "يجب تفعيل فئة واحدة على الأقل.", w);
    catHelp->setWordWrap(true);
    catHelp->setStyleSheet("color: gray; font-size: 11px; margin-bottom: 16px;");
    vl->addWidget(catHelp);

    vl->addSpacing(12);

    // Shuffle
    m_shuffleChk = new QCheckBox("ترتيب عشوائي");
    addFormRow(w, vl, "ترتيب العرض", m_shuffleChk,
               "عند التفعيل، تُعرض الأذكار بترتيب عشوائي في كل دورة بدلًا من الترتيب الثابت. يجعل التجربة أكثر تنوعًا.");

    // Show source
    m_showSourceChk = new QCheckBox("عرض مصدر الحديث");
    addFormRow(w, vl, "المصدر", m_showSourceChk,
               "عند التفعيل، يُعرض مرجع كل ذكر (مثل: البخاري ٦٣٠٦) أسفل النص. مفيد للتحقق والمراجعة.");

    // Show translation
    m_showTransChk = new QCheckBox("عرض الترجمة الإنجليزية");
    addFormRow(w, vl, "الترجمة", m_showTransChk,
               "يعرض ترجمة إنجليزية لكل ذكر أسفل النص العربي. مفيد إذا كنت تتعلم العربية أو تشارك الأذكار مع غير الناطقين بالعربية.");

    vl->addStretch();
    scroll->setWidget(w);
    return scroll;
}

QWidget *SettingsWindow::buildAboutTab()
{
    QWidget     *w  = new QWidget;
    QVBoxLayout *vl = new QVBoxLayout(w);
    vl->setContentsMargins(32, 32, 32, 32);
    vl->setSpacing(16);

    // App title
    QLabel *title = new QLabel("أذكاري — Azkare");
    QFont tf = title->font();
    tf.setPointSize(18);
    tf.setBold(true);
    title->setFont(tf);
    title->setAlignment(Qt::AlignCenter);
    vl->addWidget(title);

    QLabel *version = new QLabel("الإصدار 1.0.0");
    version->setAlignment(Qt::AlignCenter);
    version->setStyleSheet("color: gray;");
    vl->addWidget(version);

    QLabel *desc = new QLabel(
        "تطبيق مكتبي مفتوح المصدر يعرض أكثر من ٥٠٠ ذكر ودعاء أصيل\n"
        "فوق خلفية سطح مكتبك — Linux وWindows وmacOS\n\n"
        "جميع الأذكار موثّقة من الدرر السنية، إسلام ويب، والإسلام سؤال وجواب."
    );
    desc->setAlignment(Qt::AlignCenter);
    desc->setWordWrap(true);
    vl->addWidget(desc);

    vl->addSpacing(8);

    // Developer
    QLabel *devLabel = new QLabel("المطور: Mahmoud El Sheikh (arcalc)");
    devLabel->setAlignment(Qt::AlignCenter);
    vl->addWidget(devLabel);

    // Links
    auto makeLink = [&](const QString &text, const QString &url) {
        QPushButton *btn = new QPushButton(text, w);
        btn->setFlat(true);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet("QPushButton { color: #5289AD; } QPushButton:hover { color: #3f6e90; }");
        connect(btn, &QPushButton::clicked, this, [url]() {
            QDesktopServices::openUrl(QUrl(url));
        });
        return btn;
    };

    QHBoxLayout *linksRow = new QHBoxLayout;
    linksRow->addStretch();
    linksRow->addWidget(makeLink("GitHub", "https://github.com/arcalc/azkare"));
    linksRow->addWidget(new QLabel("·"));
    linksRow->addWidget(makeLink("الموقع الشخصي", "https://arcalc.github.io/arcalc"));
    linksRow->addWidget(new QLabel("·"));
    linksRow->addWidget(makeLink("arcalc@proton.me", "mailto:arcalc@proton.me"));
    linksRow->addStretch();
    vl->addLayout(linksRow);

    vl->addSpacing(16);

    // Update check
    m_updateLabel   = new QLabel("", w);
    m_updateLabel->setAlignment(Qt::AlignCenter);
    m_updateLabel->setWordWrap(true);
    vl->addWidget(m_updateLabel);

    m_checkUpdateBtn = new QPushButton("البحث عن تحديثات");
    m_checkUpdateBtn->setMaximumWidth(220);
    connect(m_checkUpdateBtn, &QPushButton::clicked, this, &SettingsWindow::onCheckUpdate);
    QHBoxLayout *updateRow = new QHBoxLayout;
    updateRow->addStretch();
    updateRow->addWidget(m_checkUpdateBtn);
    updateRow->addStretch();
    vl->addLayout(updateRow);

    vl->addStretch();

    QLabel *license = new QLabel("مرخّص بموجب رخصة MIT — مفتوح المصدر بحرية");
    license->setAlignment(Qt::AlignCenter);
    license->setStyleSheet("color: gray; font-size: 11px;");
    vl->addWidget(license);

    return w;
}

// ── Load / Save ───────────────────────────────────────────────────────────────

void SettingsWindow::loadToUi()
{
    // General
    m_langCombo->setCurrentIndex(m_settings.language() == "ar" ? 0 : 1);
    m_autostartChk->setChecked(m_settings.autostartEnabled());

    // Display
    m_modeCombo->setCurrentIndex(int(m_settings.displayMode()));
    if (m_backendCombo) m_backendCombo->setCurrentIndex(int(m_settings.overlayBackend()));
    m_posCombo->setCurrentIndex(int(m_settings.overlayPosition()));
    m_monitorCombo->setCurrentIndex(m_settings.overlayMonitor());
    m_rotationSpin->setValue(m_settings.rotationIntervalSecs());

    // Appearance
    m_fontCombo->setCurrentFont(QFont(m_settings.fontFamily()));
    m_fontSizeSpin->setValue(m_settings.fontSize());
    m_shadowChk->setChecked(m_settings.shadowEnabled());

    QColor c = m_settings.textColor();
    m_colorBtn->setStyleSheet(
        QString("background-color: %1; color: %2; padding: 4px 16px;")
            .arg(c.name())
            .arg(c.lightness() > 128 ? "#000" : "#fff"));

    switch (int(m_settings.textAlignment())) {
    case int(Qt::AlignRight):  m_alignCombo->setCurrentIndex(0); break;
    case int(Qt::AlignCenter): m_alignCombo->setCurrentIndex(1); break;
    case int(Qt::AlignLeft):   m_alignCombo->setCurrentIndex(2); break;
    default: break;
    }

    // Notifications
    m_notifEnabledChk->setChecked(m_settings.notificationsEnabled());
    m_notifIntervalSpin->setValue(m_settings.notificationIntervalSecs());

    // Content
    QStringList activeCats = m_settings.activeCategories();
    for (QCheckBox *chk : m_categoryChks)
        chk->setChecked(activeCats.isEmpty() || activeCats.contains(chk->objectName()));

    m_shuffleChk->setChecked(m_settings.shuffleEnabled());
    m_showSourceChk->setChecked(m_settings.showSource());
    m_showTransChk->setChecked(m_settings.showTranslation());
}

void SettingsWindow::saveFromUi()
{
    m_settings.setLanguage(m_langCombo->currentData().toString());
    m_settings.setAutostartEnabled(m_autostartChk->isChecked());

    m_settings.setDisplayMode(DisplayMode(m_modeCombo->currentData().toInt()));
    if (m_backendCombo)
        m_settings.setOverlayBackend(OverlayBackend(m_backendCombo->currentData().toInt()));
    m_settings.setOverlayPosition(OverlayPosition(m_posCombo->currentData().toInt()));
    m_settings.setOverlayMonitor(m_monitorCombo->currentIndex());
    m_settings.setRotationIntervalSecs(m_rotationSpin->value());

    m_settings.setFontFamily(m_fontCombo->currentFont().family());
    m_settings.setFontSize(m_fontSizeSpin->value());
    m_settings.setShadowEnabled(m_shadowChk->isChecked());

    static const Qt::Alignment aligns[] = {Qt::AlignRight, Qt::AlignCenter, Qt::AlignLeft};
    m_settings.setTextAlignment(aligns[m_alignCombo->currentIndex()]);

    m_settings.setNotificationsEnabled(m_notifEnabledChk->isChecked());
    m_settings.setNotificationIntervalSecs(m_notifIntervalSpin->value());

    QStringList cats;
    for (QCheckBox *chk : m_categoryChks)
        if (chk->isChecked()) cats << chk->objectName();
    m_settings.setActiveCategories(cats);

    m_settings.setShuffleEnabled(m_shuffleChk->isChecked());
    m_settings.setShowSource(m_showSourceChk->isChecked());
    m_settings.setShowTranslation(m_showTransChk->isChecked());
}

// ── Slots ─────────────────────────────────────────────────────────────────────

void SettingsWindow::onSave()
{
    saveFromUi();
    m_settings.save();

    // Apply autostart change
    AutostartManager autostart;
    autostart.setEnabled(m_settings.autostartEnabled());

    // Push new settings live
    m_db.setActiveCategories(m_settings.activeCategories());
    m_db.setShuffleEnabled(m_settings.shuffleEnabled());
    m_overlay.applySettings();
    m_scheduler.applyNewSettings();

    hide();
}

void SettingsWindow::onApply()
{
    saveFromUi();
    m_settings.save();
    m_db.setActiveCategories(m_settings.activeCategories());
    m_db.setShuffleEnabled(m_settings.shuffleEnabled());
    m_overlay.applySettings();
    m_scheduler.applyNewSettings();
}

void SettingsWindow::onCancel()
{
    loadToUi(); // Revert UI
    hide();
}

void SettingsWindow::onLanguageChanged(int) {}
void SettingsWindow::onDisplayModeChanged(int) {}
void SettingsWindow::onBackendChanged(int) {}
void SettingsWindow::onPositionChanged(int) {}

void SettingsWindow::onDragModeToggled(bool checked)
{
    // Live-preview drag mode on the overlay widget
    m_overlay.setVisible(true);
    // OverlayManager needs a setDragMode method — forwarded here
    // m_overlay.setDragMode(checked);
}

void SettingsWindow::onFontChanged(const QFont &) {}

void SettingsWindow::onColorPickerClicked()
{
    QColor c = QColorDialog::getColor(
        m_settings.textColor(), this, "اختر لون النص",
        QColorDialog::ShowAlphaChannel);
    if (c.isValid()) {
        m_settings.setTextColor(c);
        m_colorBtn->setStyleSheet(
            QString("background-color: %1; color: %2; padding: 4px 16px;")
                .arg(c.name())
                .arg(c.lightness() > 128 ? "#000" : "#fff"));
    }
}

void SettingsWindow::onCheckUpdate()
{
    if (m_updateLabel)
        m_updateLabel->setText("جارٍ البحث...");

    Updater *updater = new Updater("arcalc", "azkare", "1.0.0", this);
    connect(updater, &Updater::updateAvailable, this, [this](const ReleaseInfo &info) {
        if (m_updateLabel)
            m_updateLabel->setText(
                QString("يوجد تحديث جديد: %1\n%2").arg(info.tag, info.name));
        m_tray.showUpdateNotification(info);
    });
    connect(updater, &Updater::upToDate, this, [this]() {
        if (m_updateLabel)
            m_updateLabel->setText("أنت تستخدم أحدث إصدار.");
    });
    connect(updater, &Updater::checkFailed, this, [this](const QString &reason) {
        if (m_updateLabel)
            m_updateLabel->setText("تعذّر التحقق: " + reason);
    });
    updater->checkAsync();
}

void SettingsWindow::retranslate() {}
