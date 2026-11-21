#pragma once

#include <QMainWindow>
#include <QTabWidget>
#include <QComboBox>
#include <QCheckBox>
#include <QSlider>
#include <QSpinBox>
#include <QFontComboBox>
#include <QToolButton>
#include <QLabel>
#include <QPushButton>

class AppSettings;
class AdhkarDatabase;
class OverlayManager;
class NotificationDispatcher;
class Scheduler;
class TrayController;

/**
 * Full settings window with tabs:
 *   General | Display | Notifications | Appearance | Content | About
 *
 * Every control has a plain-language helper label directly beneath it
 * so non-technical users understand each option.
 */
class SettingsWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit SettingsWindow(AppSettings            &settings,
                            AdhkarDatabase         &db,
                            OverlayManager         &overlay,
                            NotificationDispatcher &notifier,
                            Scheduler              &scheduler,
                            TrayController         &tray,
                            QWidget                *parent = nullptr);

    void showAndRaise();

private slots:
    void onSave();
    void onCancel();
    void onApply();
    void onLanguageChanged(int idx);
    void onDisplayModeChanged(int idx);
    void onBackendChanged(int idx);
    void onPositionChanged(int idx);
    void onDragModeToggled(bool checked);
    void onFontChanged(const QFont &font);
    void onColorPickerClicked();
    void onCheckUpdate();

private:
    // ── Tab builders ──────────────────────────────────────────────────────
    QWidget *buildGeneralTab();
    QWidget *buildDisplayTab();
    QWidget *buildNotificationsTab();
    QWidget *buildAppearanceTab();
    QWidget *buildContentTab();
    QWidget *buildAboutTab();

    /** Add a labeled control + helper subtitle in a consistent layout. */
    void addFormRow(QWidget *container, QLayout *layout,
                    const QString &label,
                    QWidget       *control,
                    const QString &helpText);

    void loadToUi();
    void saveFromUi();
    void retranslate();

    // ── References ────────────────────────────────────────────────────────
    AppSettings            &m_settings;
    AdhkarDatabase         &m_db;
    OverlayManager         &m_overlay;
    NotificationDispatcher &m_notifier;
    Scheduler              &m_scheduler;
    TrayController         &m_tray;

    // ── Tab widget ────────────────────────────────────────────────────────
    QTabWidget *m_tabs = nullptr;

    // ── General controls ──────────────────────────────────────────────────
    QComboBox *m_langCombo     = nullptr;
    QCheckBox *m_autostartChk  = nullptr;

    // ── Display controls ──────────────────────────────────────────────────
    QComboBox *m_modeCombo     = nullptr;
    QComboBox *m_backendCombo  = nullptr;
    QComboBox *m_posCombo      = nullptr;
    QComboBox *m_monitorCombo  = nullptr;
    QSpinBox  *m_rotationSpin  = nullptr;
    QCheckBox *m_dragModeChk   = nullptr;

    // ── Appearance controls ───────────────────────────────────────────────
    QFontComboBox *m_fontCombo   = nullptr;
    QSpinBox      *m_fontSizeSpin = nullptr;
    QToolButton   *m_colorBtn    = nullptr;
    QCheckBox     *m_shadowChk   = nullptr;
    QComboBox     *m_alignCombo  = nullptr;

    // ── Notification controls ─────────────────────────────────────────────
    QCheckBox *m_notifEnabledChk = nullptr;
    QSpinBox  *m_notifIntervalSpin = nullptr;

    // ── Content controls ──────────────────────────────────────────────────
    QList<QCheckBox *> m_categoryChks;
    QCheckBox *m_shuffleChk      = nullptr;
    QCheckBox *m_showSourceChk   = nullptr;
    QCheckBox *m_showTransChk    = nullptr;

    // ── About ─────────────────────────────────────────────────────────────
    QLabel    *m_updateLabel     = nullptr;
    QPushButton *m_checkUpdateBtn = nullptr;
};
