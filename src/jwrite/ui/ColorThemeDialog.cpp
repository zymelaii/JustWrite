#include <jwrite/ui/ColorThemeDialog.h>
#include <jwrite/ui/ColorSelectorButton.h>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>

namespace jwrite::ui {

ColorThemeDialog::ColorThemeDialog(QWidget *parent)
    : QDialog(parent) {
    setupUi();
}

ColorThemeDialog::ColorThemeDialog(ColorTheme theme, QWidget *parent)
    : QDialog(parent)
    , theme_{theme} {
    setupUi();
}

ColorThemeDialog::~ColorThemeDialog() {}

void ColorThemeDialog::setupUi() {
    theme_ = ColorTheme::from_schema(ColorSchema::Dark);

    auto layout = new QVBoxLayout(this);

    auto schema_container = new QWidget;
    auto layout_schema    = new QHBoxLayout(schema_container);
    ui_schema_select_     = new QComboBox;
    auto schema_label     = new QLabel("基准颜色模式");
    layout_schema->addWidget(schema_label);
    layout_schema->addWidget(ui_schema_select_);

    auto color_picker_conatiner = new QWidget;
    auto layout_color_picker    = new QGridLayout(color_picker_conatiner);

    QList<QPair<QString, QColor *>> color_entries{
        {"背景",       &theme_.Window      },
        {"前景",       &theme_.WindowText  },
        {"边框",       &theme_.Border      },
        {"面板",       &theme_.Panel       },
        {"文本",       &theme_.Text        },
        {"文本背景", &theme_.TextBase    },
        {"选中文本", &theme_.SelectedText},
        {"悬停",       &theme_.Hover       },
        {"选中项",    &theme_.SelectedItem},
    };

    int index = 0;
    for (auto [desc, color_ref] : color_entries) {
        auto container        = new QWidget;
        auto layout_container = new QHBoxLayout(container);
        auto label            = new QLabel(desc);
        auto selector         = new ColorSelectorButton;
        layout_container->addWidget(selector);
        layout_container->addWidget(label);
        layout_color_picker->addWidget(container, index / 2, index % 2);

        selector->setFixedWidth(40);
        selector->setColor(*color_ref);

        connect(selector, &ColorSelectorButton::colorChanged, [&ref = *color_ref](QColor color) {
            ref = color;
        });
        connect(
            this, &ColorThemeDialog::themeChanged, selector, [this, selector, &ref = *color_ref] {
                blockSignals(true);
                selector->setColor(ref);
                blockSignals(false);
            });

        ++index;
    }

    auto button_container = new QWidget;
    auto layout_button    = new QHBoxLayout(button_container);
    auto ok_button        = new QPushButton("确定");
    auto apply_button     = new QPushButton("应用");
    auto cancel_button    = new QPushButton("取消");
    layout_button->addStretch();
    layout_button->addWidget(ok_button);
    layout_button->addWidget(apply_button);
    layout_button->addWidget(cancel_button);

    layout->addWidget(schema_container);
    layout->addWidget(color_picker_conatiner);
    layout->addWidget(button_container);

    connect(ok_button, SIGNAL(clicked()), this, SLOT(accept()));
    connect(apply_button, SIGNAL(clicked()), this, SLOT(notifyThemeApplied()));
    connect(cancel_button, SIGNAL(clicked()), this, SLOT(reject()));
    connect(
        ui_schema_select_, SIGNAL(currentIndexChanged(int)), this, SLOT(notifyThemeChanged(int)));

    ui_schema_select_->addItem("亮色", QVariant::fromValue(ColorSchema::Light));
    ui_schema_select_->addItem("暗色", QVariant::fromValue(ColorSchema::Dark));
}

void ColorThemeDialog::notifyThemeChanged(int schema_index) {
    const auto schema = ui_schema_select_->itemData(schema_index).value<ColorSchema>();
    theme_            = ColorTheme::from_schema(schema);
    emit themeChanged();
}

void ColorThemeDialog::notifyThemeApplied() {
    emit themeApplied();
}

} // namespace jwrite::ui
