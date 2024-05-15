#include <QThread>
#include <atomic>

namespace jwrite {

class MessyInputWorker : public QThread {
public:
    explicit MessyInputWorker(QObject *parent = nullptr)
        : QThread(parent)
        , enabled_(false) {}

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

} // namespace jwrite
