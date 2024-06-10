#pragma once

#include <QWidget>
#include <QEventLoop>

namespace widgetkit {

class OverlaySurface;

class OverlayDialog : public QWidget {
    Q_OBJECT

public:
    OverlayDialog();
    virtual ~OverlayDialog();

signals:
    void accepted();
    void rejected();

public slots:
    void accept();
    void reject();
    void quit();
    void exit(int result);

public:
    virtual int exec(OverlaySurface *surface);

    QWidget *surface() const {
        return surface_;
    }

    bool isAccepted() const {
        return accepted_or_rejected_ ? accepted_or_rejected_.value() : false;
    }

    bool isRejected() const {
        return accepted_or_rejected_ ? !accepted_or_rejected_.value() : false;
    }

protected:
    void setResult(int result) {
        result_ = result;
    }

    int result() const {
        return result_;
    }

    QEventLoop *&eventLoop() const {
        return const_cast<OverlayDialog *>(this)->event_loop_;
    }

private:
    QEventLoop         *event_loop_;
    int                 result_;
    std::optional<bool> accepted_or_rejected_;
    QWidget            *surface_;
};

} // namespace widgetkit
