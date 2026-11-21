#include "OverlayWidget.h"

#include <QPainter>
#include <QScreen>
#include <QGuiApplication>
#include <QMouseEvent>
#include <QFontMetrics>
#include <QTextOption>
#include <QDebug>

#if defined(Q_OS_LINUX)
#  include <QX11Info>
#  include <X11/Xlib.h>
#  include <X11/Xatom.h>
#endif

#if defined(Q_OS_WIN)
#  include <windows.h>
#endif

OverlayWidget::OverlayWidget(AppSettings &settings, QWidget *parent)
    : QWidget(parent, Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnBottomHint)
    , m_settings(settings)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
    setAttribute(Qt::WA_X11DoNotAcceptFocus);
    setWindowFlag(Qt::WindowTransparentForInput);

    // Opacity animation
    m_opacity = new QGraphicsOpacityEffect(this);
    m_opacity->setOpacity(1.0);
    setGraphicsEffect(m_opacity);

    m_fadeOut = new QPropertyAnimation(m_opacity, "opacity", this);
    m_fadeOut->setDuration(600);
    m_fadeOut->setStartValue(1.0);
    m_fadeOut->setEndValue(0.0);

    m_fadeIn = new QPropertyAnimation(m_opacity, "opacity", this);
    m_fadeIn->setDuration(600);
    m_fadeIn->setStartValue(0.0);
    m_fadeIn->setEndValue(1.0);

    // When fade-out completes, switch content and fade back in
    connect(m_fadeOut, &QPropertyAnimation::finished, this, [this]() {
        if (m_hasPending) {
            m_entry      = m_pending;
            m_hasPending = false;
            update();
            m_fadeIn->start();
        }
    });
}

void OverlayWidget::initialize()
{
    applySettings();
    show();

#if defined(Q_OS_LINUX)
    applyX11DesktopHints();
#elif defined(Q_OS_WIN)
    applyWindowsDesktopLayer();
#elif defined(Q_OS_MACOS)
    applyMacOSDesktopLevel();
#endif
}

void OverlayWidget::showEntry(const AdhkarEntry &entry)
{
    if (m_fadeOut->state() == QPropertyAnimation::Running) {
        // Already fading — queue this entry
        m_pending    = entry;
        m_hasPending = true;
        return;
    }

    if (m_entry.id == entry.id) return; // same entry — no-op

    m_pending    = entry;
    m_hasPending = true;
    m_fadeOut->start();
}

void OverlayWidget::applySettings()
{
    computePosition();
    update();
}

void OverlayWidget::setDragMode(bool enabled)
{
    m_dragMode = enabled;
    setAttribute(Qt::WA_TransparentForMouseEvents, !enabled);
    setWindowFlag(Qt::WindowTransparentForInput, !enabled);
    if (enabled)
        setCursor(Qt::SizeAllCursor);
    else
        unsetCursor();
}

// ── Painting ─────────────────────────────────────────────────────────────────

void OverlayWidget::paintEvent(QPaintEvent *)
{
    if (m_entry.textAr.isEmpty()) return;

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);

    QFont font(m_settings.fontFamily(), m_settings.fontSize());
    p.setFont(font);

    QColor color = m_settings.textColor();

    // Shadow pass
    if (m_settings.shadowEnabled()) {
        p.setPen(QColor(0, 0, 0, 160));
        QRectF shadowRect = rect().adjusted(2, 2, 2, 2);
        p.drawText(shadowRect, Qt::AlignCenter | Qt::TextWordWrap, m_entry.textAr);
    }

    // Main text
    p.setPen(color);

    QTextOption opt;
    opt.setAlignment(m_settings.textAlignment() | Qt::AlignVCenter);
    opt.setWrapMode(QTextOption::WordWrap);
    opt.setTextDirection(Qt::RightToLeft); // Arabic is always RTL

    p.drawText(rect(), m_entry.textAr, opt);

    // Optional source attribution
    if (m_settings.showSource() && !m_entry.sourceRef.isEmpty()) {
        QFont smallFont(m_settings.fontFamily(), m_settings.fontSize() / 2);
        p.setFont(smallFont);
        p.setPen(QColor(color.red(), color.green(), color.blue(), 160));
        QRect sourceRect = rect().adjusted(0, rect().height() - 30, 0, 0);
        p.drawText(sourceRect, Qt::AlignCenter, m_entry.sourceRef);
    }
}

// ── Mouse for drag mode ───────────────────────────────────────────────────────

void OverlayWidget::mousePressEvent(QMouseEvent *event)
{
    if (m_dragMode && event->button() == Qt::LeftButton) {
        m_dragging  = true;
        m_dragStart = event->globalPosition().toPoint() - frameGeometry().topLeft();
        event->accept();
    }
}

void OverlayWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragging) {
        QPoint newPos = event->globalPosition().toPoint() - m_dragStart;
        move(newPos);
        event->accept();
    }
}

void OverlayWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_dragging) {
        m_dragging = false;
        emit movedByUser(pos());
        event->accept();
    }
}

// ── Position helpers ──────────────────────────────────────────────────────────

void OverlayWidget::computePosition()
{
    QList<QScreen *> screens = QGuiApplication::screens();
    int monIdx = qBound(0, m_settings.overlayMonitor(), screens.size() - 1);
    QRect screen = screens[monIdx]->geometry();

    // Widget dimensions based on font metrics
    QFontMetrics fm(QFont(m_settings.fontFamily(), m_settings.fontSize()));
    int w = qMin(screen.width() * 3 / 4, 900);
    int h = fm.height() * 5; // generous height for multi-line
    resize(w, h);

    QPoint pos;
    switch (m_settings.overlayPosition()) {
    case OverlayPosition::TopLeft:
        pos = screen.topLeft() + QPoint(20, 20); break;
    case OverlayPosition::TopCenter:
        pos = screen.topLeft() + QPoint((screen.width() - w) / 2, 20); break;
    case OverlayPosition::TopRight:
        pos = screen.topRight() + QPoint(-w - 20, 20); break;
    case OverlayPosition::Center:
        pos = screen.center() - QPoint(w / 2, h / 2); break;
    case OverlayPosition::BottomLeft:
        pos = screen.bottomLeft() + QPoint(20, -h - 20); break;
    case OverlayPosition::BottomCenter:
        pos = screen.bottomLeft() + QPoint((screen.width() - w) / 2, -h - 20); break;
    case OverlayPosition::BottomRight:
        pos = screen.bottomRight() + QPoint(-w - 20, -h - 20); break;
    case OverlayPosition::Custom:
        pos = m_settings.overlayCustomPos(); break;
    }
    move(pos);
}

// ── Platform-specific z-order ─────────────────────────────────────────────────

#if defined(Q_OS_LINUX)
void OverlayWidget::applyX11DesktopHints()
{
    if (!QX11Info::isPlatformX11()) return;

    Display *dpy  = QX11Info::display();
    WId      wid  = winId();

    // Set _NET_WM_WINDOW_TYPE to _NET_WM_WINDOW_TYPE_DESKTOP
    Atom typeAtom    = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE",        False);
    Atom desktopAtom = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DESKTOP", False);
    XChangeProperty(dpy, wid, typeAtom, XA_ATOM, 32,
                    PropModeReplace,
                    reinterpret_cast<unsigned char *>(&desktopAtom), 1);

    // Also set _NET_WM_STATE_BELOW
    Atom stateAtom = XInternAtom(dpy, "_NET_WM_STATE",       False);
    Atom belowAtom = XInternAtom(dpy, "_NET_WM_STATE_BELOW", False);
    XChangeProperty(dpy, wid, stateAtom, XA_ATOM, 32,
                    PropModeReplace,
                    reinterpret_cast<unsigned char *>(&belowAtom), 1);

    XLowerWindow(dpy, wid);
    XFlush(dpy);

    qInfo() << "[OverlayWidget] X11 desktop hints applied.";
}
#else
void OverlayWidget::applyX11DesktopHints() {}
#endif

#if defined(Q_OS_WIN)
void OverlayWidget::applyWindowsDesktopLayer()
{
    // Insert our window into the WorkerW desktop layer.
    // 1. Find the WorkerW window that holds the desktop icons.
    HWND progman = FindWindow(L"Progman", nullptr);
    // Send 0x052C to spawn a WorkerW beneath the icon layer
    SendMessageTimeout(progman, 0x052C, 0, 0, SMTO_NORMAL, 1000, nullptr);

    HWND workerW = nullptr;
    EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
        HWND shell = FindWindowEx(hwnd, nullptr, L"SHELLDLL_DefView", nullptr);
        if (shell) {
            *reinterpret_cast<HWND *>(lParam) = FindWindowEx(nullptr, hwnd, L"WorkerW", nullptr);
            return FALSE;
        }
        return TRUE;
    }, reinterpret_cast<LPARAM>(&workerW));

    if (workerW) {
        HWND ourHwnd = reinterpret_cast<HWND>(winId());
        SetParent(ourHwnd, workerW);
        qInfo() << "[OverlayWidget] Windows WorkerW injection successful.";
    } else {
        // Fallback: layered transparent window pinned below
        HWND ourHwnd = reinterpret_cast<HWND>(winId());
        SetWindowLongPtr(ourHwnd, GWL_EXSTYLE,
            GetWindowLongPtr(ourHwnd, GWL_EXSTYLE)
            | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW);
        SetWindowPos(ourHwnd, HWND_BOTTOM, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        qWarning() << "[OverlayWidget] WorkerW not found — using layered fallback.";
    }
}
#else
void OverlayWidget::applyWindowsDesktopLayer() {}
#endif

#if defined(Q_OS_MACOS)
void OverlayWidget::applyMacOSDesktopLevel()
{
    // Delegate to Objective-C++ bridge in platform/macos/
    extern void setWindowToDesktopLevel(WId wid);
    setWindowToDesktopLevel(winId());
    qInfo() << "[OverlayWidget] macOS desktop level applied.";
}
#else
void OverlayWidget::applyMacOSDesktopLevel() {}
#endif
