#include "qt/WebView.h"
#include <QUrl>

WebView::WebView(QWidget *parent)
    : QWebEngineView(parent)
{
    // Connecter les signaux
    connect(this, &QWebEngineView::urlChanged, this, &WebView::onUrlChanged);
    connect(this, &QWebEngineView::titleChanged, this, &WebView::onTitleChanged);
    connect(this, &QWebEngineView::loadStarted, this, &WebView::onLoadStarted);
    connect(this, &QWebEngineView::loadFinished, this, &WebView::onLoadFinished);
}

void WebView::loadUrl(const QString &url)
{
    QString normalizedUrl = url;
    if (!normalizedUrl.contains("://")) {
        normalizedUrl = "https://" + normalizedUrl;
    }
    load(QUrl(normalizedUrl));
}

QString WebView::getCurrentUrl() const
{
    return url().toString();
}

QString WebView::getTitle() const
{
    return title();
}

void WebView::onUrlChanged(const QUrl &url)
{
    emit urlChanged(url.toString());
}

void WebView::onTitleChanged(const QString &title)
{
    emit titleChanged(title);
}

void WebView::onLoadStarted()
{
    emit loadStarted();
}

void WebView::onLoadFinished(bool success)
{
    emit loadFinished(success);
}

