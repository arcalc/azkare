#include "SettingsWindow.h"

#include "AppIcon.h"
#include "../core/AdhkarDatabase.h"
#include "../core/AppSettings.h"
#include "../core/Localizer.h"
#include "../core/Ipc.h"
#include "../core/Updater.h"
#include "../platform/AutostartManager.h"
#include "Version.h"

#include <QApplication>
#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QDesktopServices>
#include <QFontDatabase>
#include <QGraphicsDropShadowEffect>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScreen>
#include <QScrollArea>
#include <QSpinBox>
#include <QTabWidget>
#include <QUrl>
#include <QVBoxLayout>

namespace {
constexpr int kKeyRole = Qt::UserRole + 1;

void addItem(QComboBox *c, const char *key, int data)
{
    c->addItem(QString(), data);
    c->setItemData(c->count() - 1, QString::fromLatin1(key), kKeyRole);
}

/** Secondary text: 65% text colour + 35% window colour, so it stays readable in light and dark themes. */
void muteLabel(QLabel *l)
{
    QPalette pal = l->palette();
    const QColor t = pal.color(QPalette::WindowText), b = pal.color(QPalette::Window);
    pal.setColor(QPalette::WindowText, QColor((t.red() * 65 + b.red() * 35) / 100, (t.green() * 65 + b.green() * 35) / 100,
                                              (t.blue() * 65 + b.blue() * 35) / 100));
    l->setPalette(pal);
}

QString presetKey(int secs)
{
    switch (secs) {
    case 30: return QStringLiteral("settings.preset_30s");
    case 60: return QStringLiteral("settings.preset_1m");
    case 300: return QStringLiteral("settings.preset_5m");
    case 900: return QStringLiteral("settings.preset_15m");
    case 1800: return QStringLiteral("settings.preset_30m");
    case 3600: return QStringLiteral("settings.preset_1h");
    case 7200: return QStringLiteral("settings.preset_2h");
    }
    return QStringLiteral("settings.preset_custom");
}

void syncPreset(QComboBox *combo, int secs)
{
    int idx = combo->findData(secs);
    if (idx < 0)
        idx = combo->findData(-1);
    combo->setCurrentIndex(idx);
}
} // namespace

SettingsWindow::SettingsWindow(AppSettings &settings, AdhkarDatabase &db, Localizer &loc, Updater &updater, QWidget *parent)
    : QMainWindow(parent), m_settings(settings), m_db(db), m_loc(loc), m_updater(updater)
{
    setWindowIcon(appIcon());
    resize(800, 680);

    auto *central = new QWidget(this);
    auto *root = new QVBoxLayout(central);
    root->setContentsMargins(0, 0, 0, 0);

    m_tabs = new QTabWidget(central);
    m_tabs->addTab(buildGeneralTab(), QString());
    m_tabs->addTab(buildDisplayTab(), QString());
    m_tabs->addTab(buildAppearanceTab(), QString());
    m_tabs->addTab(buildNotificationsTab(), QString());
    m_tabs->addTab(buildContentTab(), QString());
    m_tabs->addTab(buildAboutTab(), QString());
    root->addWidget(m_tabs, 1);

    auto *bar = new QWidget(central);
    auto *bl = new QHBoxLayout(bar);
    bl->setContentsMargins(16, 10, 16, 12);
    m_btnReset = new QPushButton(bar);
    m_btnReset->setProperty("i18n", QStringLiteral("settings.btn_reset"));
    m_btnCancel = new QPushButton(bar);
    m_btnCancel->setProperty("i18n", QStringLiteral("settings.btn_cancel"));
    m_btnApply = new QPushButton(bar);
    m_btnApply->setProperty("i18n", QStringLiteral("settings.btn_apply"));
    m_btnSave = new QPushButton(bar);
    m_btnSave->setProperty("i18n", QStringLiteral("settings.btn_save"));
    m_btnSave->setDefault(true);
    bl->addWidget(m_btnReset);
    bl->addStretch(1);
    bl->addWidget(m_btnCancel);
    bl->addWidget(m_btnApply);
    bl->addWidget(m_btnSave);
    root->addWidget(bar);
    setCentralWidget(central);

    connect(m_btnSave, &QPushButton::clicked, this, &SettingsWindow::onSave);
    connect(m_btnApply, &QPushButton::clicked, this, &SettingsWindow::onApply);
    connect(m_btnCancel, &QPushButton::clicked, this, &SettingsWindow::onCancel);
    connect(m_btnReset, &QPushButton::clicked, this, &SettingsWindow::onReset);
    connect(&m_loc, &Localizer::languageChanged, this, &SettingsWindow::retranslate);

    connect(&m_updater, &Updater::updateAvailable, this, [this](const ReleaseInfo &info) {
        m_updateLabel->setText(m_loc.t(QStringLiteral("settings.about_update_available")) + QStringLiteral(" ") + info.tag
                               + QStringLiteral("\n") + m_loc.t(QStringLiteral("settings.about_update_cmd"))
                               + QStringLiteral(" ") + info.installCommand);
        m_releaseNotes->setPlainText(info.body);
    });
    connect(&m_updater, &Updater::upToDate, this, [this]() {
        m_updateLabel->setText(m_loc.t(QStringLiteral("settings.about_up_to_date")));
    });
    connect(&m_updater, &Updater::checkFailed, this, [this](const QString &why) {
        m_updateLabel->setText(m_loc.t(QStringLiteral("settings.about_check_failed")) + QStringLiteral(" ") + why);
    });

    loadToUi();
    retranslate();
}

void SettingsWindow::showAndRaise()
{
    loadToUi();
    retranslate();
    show();
    raise();
    activateWindow();
}

void SettingsWindow::hideEvent(QHideEvent *e)
{
    if (m_dragChk && m_dragChk->isChecked())
        m_dragChk->setChecked(false);   // also tells the daemon to leave drag mode
    QMainWindow::hideEvent(e);
}

// ── Builders ─────────────────────────────────────────────────────────────────

QWidget *SettingsWindow::makePage(QVBoxLayout *&layout)
{
    auto *scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    auto *w = new QWidget;
    layout = new QVBoxLayout(w);
    layout->setContentsMargins(24, 20, 24, 20);
    layout->setSpacing(4);
    scroll->setWidget(w);
    return scroll;
}

void SettingsWindow::addRow(QVBoxLayout *vl, const char *titleKey, QWidget *control, const char *helpKey)
{
    QWidget *parent = vl->parentWidget();
    if (titleKey) {
        auto *t = new QLabel(parent);
        t->setProperty("i18n", QString::fromLatin1(titleKey));
        QFont f = t->font();
        f.setBold(true);
        t->setFont(f);
        vl->addWidget(t);
    }
    if (control)
        vl->addWidget(control);
    if (helpKey) {
        auto *h = new QLabel(parent);
        h->setProperty("i18n", QString::fromLatin1(helpKey));
        h->setWordWrap(true);
        muteLabel(h);
        vl->addWidget(h);
    }
    vl->addSpacing(14);
}

QWidget *SettingsWindow::makeIntervalPicker(QComboBox *&combo, QSpinBox *&spin, const QList<int> &presets, int minSecs)
{
    auto *w = new QWidget;
    auto *hl = new QHBoxLayout(w);
    hl->setContentsMargins(0, 0, 0, 0);
    combo = new QComboBox(w);
    for (int p : presets)
        addItem(combo, presetKey(p).toLatin1().constData(), p);
    addItem(combo, "settings.preset_custom", -1);
    spin = new QSpinBox(w);
    spin->setRange(minSecs, 86400);
    spin->setProperty("i18nSuffix", QStringLiteral("settings.unit_seconds"));
    hl->addWidget(combo, 1);
    hl->addWidget(spin, 1);
    QComboBox *c = combo;
    QSpinBox *s = spin;
    connect(c, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this, c, s](int) {
        const int v = c->currentData().toInt();
        if (!m_loading && v > 0)
            s->setValue(v);
    });
    connect(s, QOverload<int>::of(&QSpinBox::valueChanged), this, [c](int v) { syncPreset(c, v); });
    return w;
}

QWidget *SettingsWindow::buildGeneralTab()
{
    QVBoxLayout *vl;
    QWidget *page = makePage(vl);
    m_langCombo = new QComboBox;
    m_langCombo->addItem(QString::fromUtf8("العربية"), QStringLiteral("ar"));
    m_langCombo->addItem(QStringLiteral("English"), QStringLiteral("en"));
    connect(m_langCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
        if (m_loading)
            return;
        const QString code = m_langCombo->currentData().toString();
        m_settings.setLanguage(code);
        m_loc.load(code);
    });
    addRow(vl, "settings.general_language", m_langCombo, "settings.general_language_desc");

    m_autostartChk = new QCheckBox;
    m_autostartChk->setProperty("i18n", QStringLiteral("settings.general_autostart_label"));
    addRow(vl, "settings.general_autostart", m_autostartChk, "settings.general_autostart_desc");
    vl->addStretch(1);
    return page;
}

QWidget *SettingsWindow::buildDisplayTab()
{
    QVBoxLayout *vl;
    QWidget *page = makePage(vl);

    m_modeCombo = new QComboBox;
    addItem(m_modeCombo, "settings.overlay_mode_desktop_only", int(DisplayMode::OverlayOnly));
    addItem(m_modeCombo, "settings.overlay_mode_notifications_only", int(DisplayMode::NotificationsOnly));
    addItem(m_modeCombo, "settings.overlay_mode_both", int(DisplayMode::Both));
    addRow(vl, "settings.overlay_mode", m_modeCombo, "settings.overlay_mode_desc");

#if defined(Q_OS_LINUX)
    m_backendCombo = new QComboBox;
    addItem(m_backendCombo, "settings.backend_native", int(OverlayBackend::Native));
    addItem(m_backendCombo, "settings.backend_eww", int(OverlayBackend::Eww));
    addRow(vl, "settings.overlay_backend", m_backendCombo, "settings.overlay_backend_desc");
#endif

    m_posCombo = new QComboBox;
    addItem(m_posCombo, "settings.position_topleft", int(OverlayPosition::TopLeft));
    addItem(m_posCombo, "settings.position_topcenter", int(OverlayPosition::TopCenter));
    addItem(m_posCombo, "settings.position_topright", int(OverlayPosition::TopRight));
    addItem(m_posCombo, "settings.position_center", int(OverlayPosition::Center));
    addItem(m_posCombo, "settings.position_bottomleft", int(OverlayPosition::BottomLeft));
    addItem(m_posCombo, "settings.position_bottomcenter", int(OverlayPosition::BottomCenter));
    addItem(m_posCombo, "settings.position_bottomright", int(OverlayPosition::BottomRight));
    addItem(m_posCombo, "settings.position_custom", int(OverlayPosition::Custom));
    addRow(vl, "settings.overlay_position", m_posCombo, "settings.overlay_position_desc");

    m_dragChk = new QCheckBox;
    m_dragChk->setProperty("i18n", QStringLiteral("settings.overlay_drag_mode_label"));
    connect(m_dragChk, &QCheckBox::toggled, this, &SettingsWindow::onDragToggled);
    addRow(vl, "settings.overlay_drag_mode", m_dragChk, "settings.overlay_drag_mode_desc");

    m_monitorCombo = new QComboBox;
    const auto screens = QGuiApplication::screens();
    for (int i = 0; i < screens.size(); ++i)
        m_monitorCombo->addItem(QStringLiteral("%1 — %2×%3").arg(i + 1).arg(screens[i]->geometry().width()).arg(screens[i]->geometry().height()), i);
    addRow(vl, "settings.overlay_monitor", m_monitorCombo, "settings.overlay_monitor_desc");

    addRow(vl, "settings.overlay_timer", makeIntervalPicker(m_rotPreset, m_rotSpin, {30, 60, 300, 900, 3600}, 5), "settings.overlay_timer_desc");
    vl->addStretch(1);
    return page;
}

QWidget *SettingsWindow::buildAppearanceTab()
{
    QVBoxLayout *vl;
    QWidget *page = makePage(vl);

    m_fontCombo = new QComboBox;
    m_fontCombo->addItems(QFontDatabase::families(QFontDatabase::Arabic));
    connect(m_fontCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SettingsWindow::updatePreview);
    addRow(vl, "settings.appearance_font", m_fontCombo, "settings.appearance_font_desc");

    m_sizeSpin = new QSpinBox;
    m_sizeSpin->setRange(8, 96);
    m_sizeSpin->setSuffix(QStringLiteral(" pt"));
    connect(m_sizeSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &SettingsWindow::updatePreview);
    addRow(vl, "settings.appearance_font_size", m_sizeSpin, "settings.appearance_font_size_desc");

    m_colorBtn = new QPushButton;
    m_colorBtn->setProperty("i18n", QStringLiteral("settings.appearance_color_btn"));
    connect(m_colorBtn, &QPushButton::clicked, this, &SettingsWindow::onPickColor);
    addRow(vl, "settings.appearance_color", m_colorBtn, "settings.appearance_color_desc");

    m_shadowChk = new QCheckBox;
    m_shadowChk->setProperty("i18n", QStringLiteral("settings.appearance_shadow_label"));
    connect(m_shadowChk, &QCheckBox::toggled, this, &SettingsWindow::updatePreview);
    addRow(vl, "settings.appearance_shadow", m_shadowChk, "settings.appearance_shadow_desc");

    m_alignCombo = new QComboBox;
    addItem(m_alignCombo, "settings.align_right", int(Qt::AlignRight));
    addItem(m_alignCombo, "settings.align_center", int(Qt::AlignHCenter));
    addItem(m_alignCombo, "settings.align_left", int(Qt::AlignLeft));
    connect(m_alignCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SettingsWindow::updatePreview);
    addRow(vl, "settings.appearance_alignment", m_alignCombo, "settings.appearance_alignment_desc");

    m_preview = new QLabel(QString::fromUtf8("سُبْحَانَ اللَّهِ وَبِحَمْدِهِ"));
    m_preview->setMinimumHeight(110);
    m_preview->setWordWrap(true);
    m_preview->setLayoutDirection(Qt::RightToLeft);
    addRow(vl, "settings.appearance_preview", m_preview, nullptr);
    vl->addStretch(1);
    return page;
}

QWidget *SettingsWindow::buildNotificationsTab()
{
    QVBoxLayout *vl;
    QWidget *page = makePage(vl);
    auto *info = new QLabel;
    info->setProperty("i18n", QStringLiteral("settings.notifications_info"));
    info->setWordWrap(true);
    addRow(vl, nullptr, info, nullptr);
    addRow(vl, "settings.notifications_interval", makeIntervalPicker(m_notifPreset, m_notifSpin, {300, 900, 1800, 3600, 7200}, 30),
           "settings.notifications_interval_desc");
    vl->addStretch(1);
    return page;
}

QWidget *SettingsWindow::buildContentTab()
{
    QVBoxLayout *vl;
    QWidget *page = makePage(vl);

    auto *box = new QGroupBox;
    box->setProperty("i18n", QStringLiteral("settings.content_categories"));
    auto *bl = new QVBoxLayout(box);
    for (const AdhkarCategory &c : m_db.categories()) {
        auto *chk = new QCheckBox(box);
        chk->setObjectName(c.id);
        bl->addWidget(chk);
        m_categoryChks << chk;
    }
    addRow(vl, nullptr, box, "settings.content_categories_desc");

    m_shuffleChk = new QCheckBox;
    m_shuffleChk->setProperty("i18n", QStringLiteral("settings.content_shuffle_label"));
    addRow(vl, "settings.content_shuffle", m_shuffleChk, "settings.content_shuffle_desc");
    m_sourceChk = new QCheckBox;
    m_sourceChk->setProperty("i18n", QStringLiteral("settings.content_show_source_label"));
    addRow(vl, "settings.content_show_source", m_sourceChk, "settings.content_show_source_desc");
    m_translationChk = new QCheckBox;
    m_translationChk->setProperty("i18n", QStringLiteral("settings.content_show_translation_label"));
    addRow(vl, "settings.content_show_translation", m_translationChk, "settings.content_show_translation_desc");
    vl->addStretch(1);
    return page;
}

QWidget *SettingsWindow::buildAboutTab()
{
    QVBoxLayout *vl;
    QWidget *page = makePage(vl);
    QWidget *host = vl->parentWidget();

    auto *title = new QLabel(host);
    title->setProperty("i18n", QStringLiteral("app.name"));
    QFont tf = title->font();
    tf.setPointSize(tf.pointSize() + 8);
    tf.setBold(true);
    title->setFont(tf);
    title->setAlignment(Qt::AlignCenter);
    vl->addWidget(title);

    auto *ver = new QLabel(QString::fromLatin1(AZKARE_VERSION_FULL), host);
    ver->setAlignment(Qt::AlignCenter);
    muteLabel(ver);
    vl->addWidget(ver);

    auto *desc = new QLabel(host);
    desc->setProperty("i18n", QStringLiteral("app.about_desc"));
    desc->setWordWrap(true);
    desc->setAlignment(Qt::AlignCenter);
    vl->addSpacing(10);
    vl->addWidget(desc);

    auto *links = new QHBoxLayout;
    links->addStretch(1);
    auto addLink = [&](const QString &text, const QString &url) {
        auto *b = new QPushButton(text, host);
        b->setFlat(true);
        b->setCursor(Qt::PointingHandCursor);
        connect(b, &QPushButton::clicked, this, [url]() { QDesktopServices::openUrl(QUrl(url)); });
        links->addWidget(b);
    };
    addLink(QStringLiteral("GitHub"), QStringLiteral("https://github.com/arcalc/azkare"));
    addLink(QStringLiteral("arcalc.github.io/arcalc"), QStringLiteral("https://arcalc.github.io/arcalc"));
    addLink(QStringLiteral("arcalc@proton.me"), QStringLiteral("mailto:arcalc@proton.me"));
    links->addStretch(1);
    vl->addLayout(links);
    vl->addSpacing(16);

    m_updateLabel = new QLabel(host);
    m_updateLabel->setAlignment(Qt::AlignCenter);
    m_updateLabel->setWordWrap(true);
    m_updateLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    vl->addWidget(m_updateLabel);

    auto *check = new QPushButton(host);
    check->setProperty("i18n", QStringLiteral("settings.about_check_updates"));
    connect(check, &QPushButton::clicked, this, [this]() {
        m_updateLabel->setText(m_loc.t(QStringLiteral("settings.about_checking")));
        m_updater.checkAsync();
    });
    auto *row = new QHBoxLayout;
    row->addStretch(1);
    row->addWidget(check);
    row->addStretch(1);
    vl->addLayout(row);

    m_releaseNotes = new QPlainTextEdit(host);
    m_releaseNotes->setReadOnly(true);
    m_releaseNotes->setMinimumHeight(140);
    m_releaseNotes->setLayoutDirection(Qt::LeftToRight);
    vl->addSpacing(8);
    vl->addWidget(m_releaseNotes);
    vl->addStretch(1);
    return page;
}

// ── Load / save ──────────────────────────────────────────────────────────────

void SettingsWindow::loadToUi()
{
    m_loading = true;
    m_langCombo->setCurrentIndex(m_langCombo->findData(m_settings.language()));
    m_autostartChk->setChecked(AutostartManager().isEnabled());

    m_modeCombo->setCurrentIndex(m_modeCombo->findData(int(m_settings.displayMode())));
    if (m_backendCombo)
        m_backendCombo->setCurrentIndex(m_backendCombo->findData(int(m_settings.overlayBackend())));
    m_posCombo->setCurrentIndex(m_posCombo->findData(int(m_settings.overlayPosition())));
    m_monitorCombo->setCurrentIndex(qBound(0, m_settings.overlayMonitor(), m_monitorCombo->count() - 1));
    m_rotSpin->setValue(m_settings.rotationIntervalSecs());
    syncPreset(m_rotPreset, m_rotSpin->value());

    const int fi = m_fontCombo->findText(m_settings.fontFamily());
    if (fi >= 0)
        m_fontCombo->setCurrentIndex(fi);
    else {
        m_fontCombo->insertItem(0, m_settings.fontFamily());
        m_fontCombo->setCurrentIndex(0);
    }
    m_sizeSpin->setValue(m_settings.fontSize());
    m_shadowChk->setChecked(m_settings.shadowEnabled());
    m_alignCombo->setCurrentIndex(m_alignCombo->findData(int(m_settings.textAlignment() & (Qt::AlignLeft | Qt::AlignRight | Qt::AlignHCenter))));
    setColorButton(m_settings.textColor());

    m_notifSpin->setValue(m_settings.notificationIntervalSecs());
    syncPreset(m_notifPreset, m_notifSpin->value());

    const QStringList active = m_settings.activeCategories();
    for (QCheckBox *c : m_categoryChks)
        c->setChecked(active.isEmpty() || active.contains(c->objectName()));
    m_shuffleChk->setChecked(m_settings.shuffleEnabled());
    m_sourceChk->setChecked(m_settings.showSource());
    m_translationChk->setChecked(m_settings.showTranslation());
    m_loading = false;
    updatePreview();
}

void SettingsWindow::saveFromUi()
{
    m_settings.setLanguage(m_langCombo->currentData().toString());
    m_settings.setDisplayMode(DisplayMode(m_modeCombo->currentData().toInt()));
    if (m_backendCombo)
        m_settings.setOverlayBackend(OverlayBackend(m_backendCombo->currentData().toInt()));
    m_settings.setOverlayPosition(OverlayPosition(m_posCombo->currentData().toInt()));
    m_settings.setOverlayMonitor(m_monitorCombo->currentData().toInt());
    m_settings.setRotationIntervalSecs(m_rotSpin->value());
    m_settings.setFontFamily(m_fontCombo->currentText());
    m_settings.setFontSize(m_sizeSpin->value());
    m_settings.setTextColor(m_color);
    m_settings.setShadowEnabled(m_shadowChk->isChecked());
    m_settings.setTextAlignment(Qt::Alignment(m_alignCombo->currentData().toInt()));
    m_settings.setNotificationIntervalSecs(m_notifSpin->value());

    QStringList cats;
    bool all = true;
    for (QCheckBox *c : m_categoryChks) {
        if (c->isChecked()) cats << c->objectName();
        else all = false;
    }
    m_settings.setActiveCategories(all || cats.isEmpty() ? QStringList() : cats); // never leave zero categories
    m_settings.setShuffleEnabled(m_shuffleChk->isChecked());
    m_settings.setShowSource(m_sourceChk->isChecked());
    m_settings.setShowTranslation(m_translationChk->isChecked());
}

void SettingsWindow::applyAll()
{
    saveFromUi();
    m_settings.save();
    AutostartManager am;
    if (am.isEnabled() != m_autostartChk->isChecked())
        am.setEnabled(m_autostartChk->isChecked());
    Ipc::send(Ipc::daemonName(), QStringLiteral("reload")); // the running daemon re-reads settings.json
}

void SettingsWindow::onApply() { applyAll(); }
void SettingsWindow::onSave() { applyAll(); close(); }

void SettingsWindow::onCancel()
{
    m_settings.resetToDefaults();
    m_settings.load();               // back to what is on disk
    m_loc.load(m_settings.language());
    loadToUi();
    close();
}

void SettingsWindow::onReset()
{
    m_settings.resetToDefaults();
    m_loc.load(m_settings.language());
    loadToUi();
}

void SettingsWindow::onPickColor()
{
    const QColor c = QColorDialog::getColor(m_color, this);
    if (c.isValid()) {
        setColorButton(c);
        updatePreview();
    }
}

void SettingsWindow::onDragToggled(bool on)
{
    if (m_loading)
        return;
    if (on) // show the overlay with the current choices first
        applyAll();
    QString reply;
    Ipc::send(Ipc::daemonName(), on ? QStringLiteral("drag:on") : QStringLiteral("drag:off"), &reply);
    // the daemon answers "custom <x> <y>" when the overlay was moved
    const QStringList parts = reply.split(QLatin1Char(' '));
    if (!on && parts.size() == 3 && parts[0] == QLatin1String("custom")) {
        m_settings.setOverlayCustomPos(QPoint(parts[1].toInt(), parts[2].toInt()));
        m_settings.setOverlayPosition(OverlayPosition::Custom);
        m_loading = true;
        m_posCombo->setCurrentIndex(m_posCombo->findData(int(OverlayPosition::Custom)));
        m_loading = false;
        m_settings.save();
    }
}

void SettingsWindow::setColorButton(const QColor &c)
{
    m_color = c;
    m_colorBtn->setStyleSheet(QStringLiteral("QPushButton { background:%1; color:%2; padding:6px 16px; border:1px solid #888; }")
                                  .arg(c.name(), c.lightness() > 128 ? QStringLiteral("#000") : QStringLiteral("#fff")));
}

void SettingsWindow::updatePreview()
{
    if (!m_preview || m_loading)
        return;
    m_preview->setFont(QFont(m_fontCombo->currentText(), m_sizeSpin->value()));
    m_preview->setStyleSheet(QStringLiteral("QLabel { background:#243C4C; color:%1; border-radius:8px; padding:16px; }").arg(m_color.name()));
    m_preview->setAlignment(Qt::Alignment(m_alignCombo->currentData().toInt()) | Qt::AlignVCenter);
    if (m_shadowChk->isChecked()) {
        auto *fx = new QGraphicsDropShadowEffect(m_preview);
        fx->setOffset(2, 2);
        fx->setBlurRadius(4);
        fx->setColor(QColor(0, 0, 0, 200));
        m_preview->setGraphicsEffect(fx);
    } else {
        m_preview->setGraphicsEffect(nullptr);
    }
}

// ── Translation ──────────────────────────────────────────────────────────────

void SettingsWindow::retranslate()
{
    static const char *tabKeys[] = {"settings.tab_general", "settings.tab_overlay", "settings.tab_appearance",
                                    "settings.tab_notifications", "settings.tab_content", "settings.tab_about"};
    setWindowTitle(m_loc.t(QStringLiteral("app.name")) + QStringLiteral(" — ") + m_loc.t(QStringLiteral("settings.title")));
    for (int i = 0; i < 6; ++i)
        m_tabs->setTabText(i, m_loc.t(QString::fromLatin1(tabKeys[i])));

    const auto widgets = findChildren<QWidget *>();
    for (QWidget *w : widgets) {
        const QVariant key = w->property("i18n");
        if (key.isValid()) {
            const QString text = m_loc.t(key.toString());
            if (auto *l = qobject_cast<QLabel *>(w)) l->setText(text);
            else if (auto *b = qobject_cast<QAbstractButton *>(w)) b->setText(text);
            else if (auto *g = qobject_cast<QGroupBox *>(w)) g->setTitle(text);
        }
        const QVariant suffix = w->property("i18nSuffix");
        if (suffix.isValid())
            if (auto *sp = qobject_cast<QSpinBox *>(w))
                sp->setSuffix(QStringLiteral(" ") + m_loc.t(suffix.toString()));
        if (auto *c = qobject_cast<QComboBox *>(w))
            for (int i = 0; i < c->count(); ++i) {
                const QVariant ik = c->itemData(i, kKeyRole);
                if (ik.isValid())
                    c->setItemText(i, m_loc.t(ik.toString()));
            }
    }
    for (QCheckBox *c : m_categoryChks)
        c->setText(m_db.categoryName(c->objectName(), m_loc.language()));

    QApplication::setLayoutDirection(m_loc.layoutDirection());
    m_preview->setLayoutDirection(Qt::RightToLeft);
    m_releaseNotes->setLayoutDirection(Qt::LeftToRight);
}
