#include <QWidget>
#include <QPalette>
#include "ParaBlockEdit.h"

struct FrontEditorPrivate;

class FrontEditor : public QWidget {
    Q_OBJECT

public:
    explicit FrontEditor(QWidget *parent = nullptr);
    virtual ~FrontEditor();

    void newParagraph();
    void createAndFocusNewParagraph();
    void enterNextParaBlock();

protected:
    [[nodiscard]] ParaBlockEdit *createParaBlock();
    void                         setActiveParaBlock(int index);
    void                         trackActiveParaBlock(ParaBlockEdit *target);
    void                         trackInactiveParaBlock(ParaBlockEdit *target);

protected:
    void focusInEvent(QFocusEvent *e) override;
    void focusOutEvent(QFocusEvent *e) override;

private:
    FrontEditorPrivate *d;
};
