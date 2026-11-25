#include "OverlayWidget.h"

#include <QAbstractTextDocumentLayout>
#include <QGraphicsOpacityEffect>
#include <QGuiApplication>
#include <QMouseEvent>
#include <QPainter>
#include <QPropertyAnimation>
#include <QScreen>
#include <QTextOption>
#include <QDebug>
#include <cmath>

#if defined(AZKARE_HAS_LAYERSHELL)
#  include <LayerShellQt/Window>
#endif
#if defined(Q_OS_MACOS)
#  include "../platform/macos/MacOverlay.h"
#endif
#if defined(Q_OS_WIN)
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  include <windows.h>
#endif
// X11 headers define macros (None, Bool, Status...) - keep them last.
#if defined(Q_OS_LINUX) && defined(AZKARE_HAS_X11)
#  include <QtGui/qguiapplication_platform.h>
#  include <X11/Xatom.h>
#  include <X11/Xlib.h>
#endif

namespace { constexpr int kPad = 12; }

OverlayWidget::OverlayWidget(AppSettings &settings, QWidget *parent)
    : QWidget(parent, Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnBottomHint | Qt::WindowDoesNotAcceptFocus)
    , m_settings(settings)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
    setWindowTitle(QStringLiteral("Azkare overlay"));
    m_doc.setDocumentMargin(0);

    m_opacity = new QGraphicsOpacityEffect(this);
    m_opacity->setOpacity(0.0);
    setGraphicsEffect(m_opacity);

    m_fadeOut = new QPropertyAnimation(m_opacity, "opacity", this);
    m_fadeOut->setDuration(500);
    m_fadeOut->setStartValue(1.0);
    m_fadeOut->setEndValue(0.0);
    m_fadeIn = new QPropertyAnimation(m_opacity, "opacity", this);
    m_fadeIn->setDuration(500);
    m_fadeIn->setStartValue(0.0);
    m_fadeIn->setEndValue(1.0);

    connect(m_fadeOut, &QPropertyAnimation::finished, this, [this]() {
        if (!m_hasPending)
            return;
        m_entry = m_pending;
        m_hasPending = false;
        rebuildDocument();
        layoutAndPlace();
        update();
        m_fadeIn->start();
    });
}

void OverlayWidget::initialize()
{
    rebuildDocument();
    layoutAndPlace();
    winId();               // create the native window so hints can be set before it is mapped
    applyPlatformHints();
    show();
    applyPlatformHints();  // some window managers only honour the hints once mapped
}

void OverlayWidget::showEntry(const AdhkarEntry &entry)
{
    if (!entry.isValid())
        return;
    if (m_fadeOut->state() == QAbstractAnimation::Running) {
        m_pending = entry;
        m_hasPending = true;
        return;
    }
    if (!m_entry.isValid()) { // first entry: just fade in
        m_entry = entry;
        rebuildDocument();
        layoutAndPlace();
        update();
        m_fadeIn->start();
        return;
    }
    if (m_entry.id == entry.id)
        return;
    m_pending = entry;
    m_hasPending = true;
    m_fadeOut->start();
}

void OverlayWidget::applySettings()
{
    rebuildDocument();
    layoutAndPlace();
    update();
}

void OverlayWidget::setDragMode(bool enabled)
{
    m_dragMode = enabled;
    setAttribute(Qt::WA_TransparentForMouseEvents, !enabled);
    setCursor(enabled ? Qt::SizeAllCursor : Qt::ArrowCursor);
    update();
}

// ── Content ─────────────────────────────────────────────────────────────────

void OverlayWidget::rebuildDocument()
{
    m_doc.setDefaultFont(QFont(m_settings.fontFamily(), m_settings.fontSize()));
    if (!m_entry.isValid()) {
        m_doc.setHtml(QString());
        return;
    }
    const char *align = (m_settings.textAlignment() & Qt::AlignLeft) ? "left"
                      : (m_settings.textAlignment() & Qt::AlignRight) ? "right" : "center";
    QString html = QStringLiteral("<p align='%1' dir='rtl' style='margin:0;'>%2</p>")
                       .arg(QLatin1String(align), m_entry.textAr.toHtmlEscaped());

    if (m_settings.showTranslation() && !m_entry.translationEn.isEmpty()) {
        html += QStringLiteral("<p align='%1' dir='ltr' style='margin:8px 0 0 0; font-size:%2pt;'>%3</p>")
                    .arg(QLatin1String(align)).arg(qMax(9, m_settings.fontSize() * 55 / 100))
                    .arg(m_entry.translationEn.toHtmlEscaped());
    }
    QStringList meta;
    if (m_settings.showSource() && !m_entry.sourceRef.isEmpty())
        meta << m_entry.sourceRef;
    if (m_entry.repeat > 1)
        meta << QStringLiteral("× %1").arg(m_entry.repeat);
    if (!meta.isEmpty()) {
        html += QStringLiteral("<p align='%1' dir='ltr' style='margin:8px 0 0 0; font-size:%2pt;'>%3</p>")
                    .arg(QLatin1String(align)).arg(qMax(8, m_settings.fontSize() * 45 / 100))
                    .arg(meta.join(QStringLiteral("  ·  ")).toHtmlEscaped());
    }
    m_doc.setHtml(html);
}

QPoint OverlayWidget::targetTopLeft(const QRect &s) const
{
    const int w = width(), h = height(), m = 24;
    switch (m_settings.overlayPosition()) {
    case OverlayPosition::TopLeft:      return QPoint(s.left() + m, s.top() + m);
    case OverlayPosition::TopCenter:    return QPoint(s.left() + (s.width() - w) / 2, s.top() + m);
    case OverlayPosition::TopRight:     return QPoint(s.right() - w - m + 1, s.top() + m);
    case OverlayPosition::Center:       return QPoint(s.left() + (s.width() - w) / 2, s.top() + (s.height() - h) / 2);
    case OverlayPosition::BottomLeft:   return QPoint(s.left() + m, s.bottom() - h - m + 1);
    case OverlayPosition::BottomCenter: return QPoint(s.left() + (s.width() - w) / 2, s.bottom() - h - m + 1);
    case OverlayPosition::BottomRight:  return QPoint(s.right() - w - m + 1, s.bottom() - h - m + 1);
    case OverlayPosition::Custom:       return m_settings.overlayCustomPos();
    }
    return s.topLeft();
}

void OverlayWidget::layoutAndPlace()
{
    const QList<QScreen *> screens = QGuiApplication::screens();
    if (screens.isEmpty())
        return;
    QScreen *scr = screens.at(qBound(0, m_settings.overlayMonitor(), int(screens.size()) - 1));
    const QRect sg = scr->geometry();

    const int w = qBound(360, int(sg.width() * 0.7), 1100);
    m_doc.setTextWidth(w - 2 * kPad);
    const int h = int(std::ceil(m_doc.size().height())) + 2 * kPad;
    resize(w, qMax(h, 2 * kPad + 8));

    const QPoint p = targetTopLeft(sg);
#if defined(AZKARE_HAS_LAYERSHELL)
    if (windowHandle() && QGuiApplication::platformName().startsWith(QLatin1String("wayland"))) {
        windowHandle()->setScreen(scr);
        if (auto *lw = LayerShellQt::Window::get(windowHandle()))
            lw->setMargins(QMargins(p.x() - sg.left(), p.y() - sg.top(), 0, 0));
        return;
    }
#endif
    move(p);
}

void OverlayWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::TextAntialiasing);
    if (m_dragMode) {
        p.setPen(QPen(QColor(255, 255, 255, 160), 1, Qt::DashLine));
        p.setBrush(QColor(0, 0, 0, 40));
        p.drawRoundedRect(rect().adjusted(1, 1, -1, -1), 8, 8);
    }
    QAbstractTextDocumentLayout::PaintContext ctx;
    if (m_settings.shadowEnabled()) {
        p.save();
        p.translate(kPad + 2, kPad + 2);
        ctx.palette.setColor(QPalette::Text, QColor(0, 0, 0, 190));
        m_doc.documentLayout()->draw(&p, ctx);
        p.restore();
    }
    ctx.palette.setColor(QPalette::Text, m_settings.textColor());
    p.translate(kPad, kPad);
    m_doc.documentLayout()->draw(&p, ctx);
}

void OverlayWidget::mousePressEvent(QMouseEvent *e)
{
    if (m_dragMode && e->button() == Qt::LeftButton) {
        m_dragging = true;
        m_dragOffset = e->globalPosition().toPoint() - frameGeometry().topLeft();
        e->accept();
    }
}

void OverlayWidget::mouseMoveEvent(QMouseEvent *e)
{
    if (m_dragging) {
        move(e->globalPosition().toPoint() - m_dragOffset);
        e->accept();
    }
}

void OverlayWidget::mouseReleaseEvent(QMouseEvent *e)
{
    if (m_dragging) {
        m_dragging = false;
        emit movedByUser(pos());
        e->accept();
    }
}

// ── Platform z-order ─────────────────────────────────────────────────────────

void OverlayWidget::applyPlatformHints()
{
#if defined(Q_OS_LINUX) && defined(AZKARE_HAS_X11)
    if (auto *x11 = qGuiApp->nativeInterface<QNativeInterface::QX11Application>()) {
        Display *dpy = x11->display();
        const ::Window wid = static_cast<::Window>(winId());
        Atom type = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE", False);
        Atom desktop = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DESKTOP", False);
        XChangeProperty(dpy, wid, type, XA_ATOM, 32, PropModeReplace, reinterpret_cast<unsigned char *>(&desktop), 1);
        Atom state = XInternAtom(dpy, "_NET_WM_STATE", False);
        Atom states[4] = {XInternAtom(dpy, "_NET_WM_STATE_BELOW", False), XInternAtom(dpy, "_NET_WM_STATE_STICKY", False),
                          XInternAtom(dpy, "_NET_WM_STATE_SKIP_TASKBAR", False), XInternAtom(dpy, "_NET_WM_STATE_SKIP_PAGER", False)};
        XChangeProperty(dpy, wid, state, XA_ATOM, 32, PropModeReplace, reinterpret_cast<unsigned char *>(states), 4);
        XLowerWindow(dpy, wid);
        XFlush(dpy);
    }
#endif
#if defined(AZKARE_HAS_LAYERSHELL)
    if (windowHandle() && QGuiApplication::platformName().startsWith(QLatin1String("wayland"))) {
        if (auto *lw = LayerShellQt::Window::get(windowHandle())) {
            lw->setLayer(LayerShellQt::Window::LayerBottom); // above wallpaper, below windows
            lw->setScope(QStringLiteral("azkare"));
            lw->setKeyboardInteractivity(LayerShellQt::Window::KeyboardInteractivityNone);
            lw->setExclusiveZone(-1);
            lw->setAnchors(LayerShellQt::Window::Anchors(LayerShellQt::Window::AnchorTop | LayerShellQt::Window::AnchorLeft));
        }
    }
#endif
#if defined(Q_OS_WIN)
    HWND self = reinterpret_cast<HWND>(winId());
    LONG_PTR ex = GetWindowLongPtr(self, GWL_EXSTYLE);
    SetWindowLongPtr(self, GWL_EXSTYLE, ex | WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE);
    SetWindowPos(self, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
#endif
#if defined(Q_OS_MACOS)
    azkare_setWindowToDesktopLevel(reinterpret_cast<void *>(winId()));
#endif
}
