// SPDX-License-Identifier: AGPL-3.0-only
//
// Time Warp HB-1's entry point: a machine of its own, not a mode of the Studio.

#include "timewarp/ui/app.hpp"
#include "timewarp/ui/retro_computer.hpp"

#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QPixmap>
#include <QSize>
#include <QString>
#include <QStringList>
#include <QTimer>

namespace {

/// The picture the README's `readme-screenshots` target wants, and a fixed size
/// so the shot does not depend on the machine the build ran on.
constexpr int kDefaultScreenshotWidth = 1200;
constexpr int kDefaultScreenshotHeight = 800;

/// How long to wait after `show()` before grabbing. The CRT paints its boot
/// sequence on a timer, and a grab at 0 ms would photograph a machine that has
/// not yet printed anything -- which reads as a broken machine rather than a fast
/// shutter.
constexpr int kScreenshotDelayMilliseconds = 1200;

bool parseSize(const QString& text, QSize* size) {
    const QStringList parts = text.split(QLatin1Char('x'), Qt::SkipEmptyParts);
    if (parts.size() != 2) {
        return false;
    }
    bool widthOk = false;
    bool heightOk = false;
    const int width = parts.at(0).toInt(&widthOk);
    const int height = parts.at(1).toInt(&heightOk);
    if (!widthOk || !heightOk || width <= 0 || height <= 0) {
        return false;
    }
    *size = QSize{width, height};
    return true;
}

}  // namespace

int main(int argc, char* argv[]) {
    QApplication application{argc, argv};
    timewarp::ui::applyApplicationIdentity(QStringLiteral("TimeWarpHb1"),
                                           QStringLiteral("time-warp-hb1"));

    QCommandLineParser parser;
    parser.setApplicationDescription(
        QStringLiteral("Time Warp HB-1 \u2014 the 1978 home computer."));
    parser.addHelpOption();
    parser.addVersionOption();

    const QCommandLineOption screenshotOption{
        QStringLiteral("screenshot"),
        QStringLiteral("Render the window to <path>, then exit."),
        QStringLiteral("path")};
    parser.addOption(screenshotOption);
    const QCommandLineOption sizeOption{
        QStringLiteral("size"),
        QStringLiteral("Screenshot size, as WxH. Default %1x%2.")
            .arg(kDefaultScreenshotWidth)
            .arg(kDefaultScreenshotHeight),
        QStringLiteral("WxH")};
    parser.addOption(sizeOption);
    parser.process(application);

    QSize screenshotSize{kDefaultScreenshotWidth, kDefaultScreenshotHeight};
    if (parser.isSet(sizeOption) && !parseSize(parser.value(sizeOption), &screenshotSize)) {
        std::fprintf(stderr, "time_warp_hb1: --size expects WxH, got \"%s\"\n",
                     qPrintable(parser.value(sizeOption)));
        return 2;
    }

    timewarp::ui::RetroComputerWindow window;
    if (parser.isSet(screenshotOption)) {
        window.resize(screenshotSize);
    }
    window.show();

    if (parser.isSet(screenshotOption)) {
        const QString screenshotPath = parser.value(screenshotOption);
        QTimer::singleShot(kScreenshotDelayMilliseconds, &application, [&window, screenshotPath] {
            const QPixmap shot = window.grab();
            if (!shot.save(screenshotPath)) {
                std::fprintf(stderr, "time_warp_hb1: could not write %s\n",
                             qPrintable(screenshotPath));
                QCoreApplication::exit(3);
                return;
            }
            std::fprintf(stderr, "%s: %dx%d\n", qPrintable(screenshotPath), shot.width(),
                         shot.height());
            QCoreApplication::exit(0);
        });
    }

    return QApplication::exec();
}
