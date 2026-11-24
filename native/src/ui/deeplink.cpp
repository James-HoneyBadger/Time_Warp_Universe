// SPDX-License-Identifier: AGPL-3.0-only

#include "timewarp/ui/deeplink.hpp"

#include <QByteArray>
#include <QStringList>
#include <QUrl>

namespace timewarp::ui {

namespace {

/// Decodes base64 in either alphabet. URL-safe base64 swaps `+` and `/` for `-`
/// and `_` and usually drops the padding, so the link survives being pasted into
/// prose; the standard alphabet is accepted for a link written by hand.
QString decodeBase64(const QString& value) {
    QByteArray bytes = value.toLatin1();
    for (qsizetype i = 0; i < bytes.size(); ++i) {
        if (bytes.at(i) == '-') {
            bytes[i] = '+';
        } else if (bytes.at(i) == '_') {
            bytes[i] = '/';
        }
    }
    while (bytes.size() % 4 != 0) {
        bytes.append('=');
    }
    return QString::fromUtf8(QByteArray::fromBase64(bytes));
}

/// One parameter from a query string, by exact name. Empty when absent.
QString parameterOf(const QStringList& pairs, const QString& name) {
    for (const QString& pair : pairs) {
        const int equals = pair.indexOf(QLatin1Char('='));
        const QString key = equals < 0 ? pair : pair.left(equals);
        if (key != name) {
            continue;
        }
        const QString raw = equals < 0 ? QString{} : pair.mid(equals + 1);
        return QUrl::fromPercentEncoding(raw.toUtf8());
    }
    return {};
}

}  // namespace

bool DeepLink::isEmpty() const {
    return example.isEmpty() && language.isEmpty() && source.isEmpty() &&
           notebook.isEmpty() && theme.isEmpty() && machine.isEmpty() && !run;
}

DeepLink parseDeepLink(const QString& rawQuery) {
    DeepLink link;
    QString query = rawQuery;
    if (query.startsWith(QLatin1Char('?'))) {
        query.remove(0, 1);
    }
    if (query.isEmpty()) {
        return link;
    }

    // Split on `&` before any decoding: the delimiter is not percent-encoded in
    // a query string, and splitting first means a `%26` inside a value is not
    // mistaken for a second parameter.
    const QStringList pairs = query.split(QLatin1Char('&'), Qt::KeepEmptyParts);

    const QString example = parameterOf(pairs, QStringLiteral("example"));
    if (!example.isEmpty()) {
        link.example = example;
    }

    const QString language = parameterOf(pairs, QStringLiteral("lang"));
    if (!language.isEmpty()) {
        link.language = language;
    }

    const QString code = parameterOf(pairs, QStringLiteral("code"));
    if (!code.isEmpty() && !language.isEmpty()) {
        link.source = decodeBase64(code);
    }

    const QString notebook = parameterOf(pairs, QStringLiteral("notebook"));
    if (!notebook.isEmpty()) {
        link.notebook = notebook;
    }

    const QString theme = parameterOf(pairs, QStringLiteral("theme"));
    if (!theme.isEmpty()) {
        // A theme name has spaces, and a query string writes a space as either
        // `%20` (decoded above) or `+`. Only this field normalises `+`: the
        // base64 `code` field uses `+` in its standard alphabet, so a global
        // replacement would corrupt a shared program.
        QString name = theme;
        link.theme = name.replace(QLatin1Char('+'), QLatin1Char(' '));
    }

    const QString machine = parameterOf(pairs, QStringLiteral("machine"));
    if (!machine.isEmpty()) {
        link.machine = machine.toLower();
    }

    // `run=1` is the canonical spelling; `autorun=1` is accepted because the
    // guides have used both words, and a link that matches the word a reader
    // expected must not quietly refuse to run.
    const QString run = parameterOf(pairs, QStringLiteral("run"));
    const QString autorun = parameterOf(pairs, QStringLiteral("autorun"));
    const auto isTrue = [](const QString& value) {
        return value == QLatin1String("1") ||
               value.compare(QLatin1String("true"), Qt::CaseInsensitive) == 0;
    };
    link.run = isTrue(run) || isTrue(autorun);

    return link;
}

}  // namespace timewarp::ui
