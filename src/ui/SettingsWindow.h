#pragma once
#include <QColor>
#include <QMainWindow>
#include <QList>

class QCheckBox;
class QComboBox;
class QLabel;
class QPlainTextEdit;
class QPushButton;
class QSpinBox;
class QTabWidget;
class QVBoxLayout;
class AppSettings;
class AdhkarDatabase;
class Localizer;
class Updater;

/**
 * Bilingual (Arabic-first, RTL/LTR) settings window. Every control has a plain-language
 * helper text under it. Texts are looked up through Localizer keys stored in the
 * "i18n" widget property, so switching language retranslates everything live.
 */
class SettingsWindow : public QMainWindow
{
    Q_OBJECT
public:
    SettingsWindow(AppSettings &settings, AdhkarDatabase &db, Localizer &loc, Updater &updater, QWidget *parent = nullptr);

    void showAndRaise();

protected:
    void hideEvent(QHideEvent *event) override;

private slots:
    void retranslate();
    void onSave();
    void onApply();
    void onCancel();
    void onReset();
    void onPickColor();
    void onDragToggled(bool on);
    void updatePreview();

private:
    QWidget *buildGeneralTab();
    QWidget *buildDisplayTab();
    QWidget *buildAppearanceTab();
    QWidget *buildNotificationsTab();
    QWidget *buildContentTab();
    QWidget *buildAboutTab();

    QWidget *makePage(QVBoxLayout *&layout);
    void addRow(QVBoxLayout *vl, const char *titleKey, QWidget *control, const char *helpKey);
    QWidget *makeIntervalPicker(QComboBox *&combo, QSpinBox *&spin, const QList<int> &presets, int minSecs);
    void applyAll();
    void loadToUi();
    void saveFromUi();
    void setColorButton(const QColor &c);

    AppSettings &m_settings;
    AdhkarDatabase &m_db;
    Localizer &m_loc;
    Updater &m_updater;

    bool m_loading = false;
    QColor m_color;
    QTabWidget *m_tabs = nullptr;

    QComboBox *m_langCombo = nullptr;
    QCheckBox *m_autostartChk = nullptr;
    QComboBox *m_modeCombo = nullptr, *m_backendCombo = nullptr, *m_posCombo = nullptr, *m_monitorCombo = nullptr;
    QCheckBox *m_dragChk = nullptr;
    QComboBox *m_rotPreset = nullptr;
    QSpinBox *m_rotSpin = nullptr;
    QComboBox *m_fontCombo = nullptr, *m_alignCombo = nullptr;
    QSpinBox *m_sizeSpin = nullptr;
    QPushButton *m_colorBtn = nullptr;
    QCheckBox *m_shadowChk = nullptr;
    QLabel *m_preview = nullptr;
    QComboBox *m_notifPreset = nullptr;
    QSpinBox *m_notifSpin = nullptr;
    QList<QCheckBox *> m_categoryChks;
    QCheckBox *m_shuffleChk = nullptr, *m_sourceChk = nullptr, *m_translationChk = nullptr;
    QLabel *m_updateLabel = nullptr;
    QPlainTextEdit *m_releaseNotes = nullptr;
    QPushButton *m_btnSave = nullptr, *m_btnApply = nullptr, *m_btnCancel = nullptr, *m_btnReset = nullptr;
};
