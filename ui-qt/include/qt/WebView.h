#ifndef WEBVIEW_H
#define WEBVIEW_H

#include <QWebEngineView>
#include <QString>

class WebView : public QWebEngineView
{
    Q_OBJECT

public:
    explicit WebView(QWidget *parent = nullptr);
    
    void loadUrl(const QString &url);
    QString getCurrentUrl() const;
    QString getTitle() const;

signals:
    void urlChanged(const QString &url);
    void titleChanged(const QString &title);
    void loadStarted();
    void loadFinished(bool success);

private slots:
    void onUrlChanged(const QUrl &url);
    void onTitleChanged(const QString &title);
    void onLoadStarted();
    void onLoadFinished(bool success);
};

#endif // WEBVIEW_H

