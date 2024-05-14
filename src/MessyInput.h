#include <QThread>
#include <atomic>

class MessyInputWorker : public QThread {
public:
    explicit MessyInputWorker(QObject *parent = nullptr);

    void start();
    void kill();

    bool is_running() const {
        return enabled_;
    }

protected:
    void send_ime_key(int key);
    void run() override;

private:
    std::atomic_bool enabled_;
};
