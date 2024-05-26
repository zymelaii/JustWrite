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
    void exit(int result = 0);

public:
    int exec(OverlaySurface *surface);

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

private:
    QEventLoop         *event_loop_;
    int                 result_;
    std::optional<bool> accepted_or_rejected_;
};

} // namespace widgetkit
