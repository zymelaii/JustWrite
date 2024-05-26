#include <jwrite/ui/ColorSchemeDialog.h>
#include <widget-kit/ColorPreviewItem.h>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QFrame>
#include <QComboBox>
#include <QCheckBox>
#include <magic_enum.hpp>

namespace jwrite::ui {

ColorSchemeDialog::ColorSchemeDialog(
    ColorTheme initial_theme, const ColorScheme &initial_scheme, QWidget *parent)
    : QDialog(parent)
    , theme_{initial_theme}
    , schemes_{{theme_, initial_scheme}} {
    setupUi();
    updateGeometry();
}

ColorSchemeDialog::~ColorSchemeDialog() {}

void ColorSchemeDialog::setupUi() {
    auto layout = new QVBoxLayout(this);

    auto theme_container        = new QWidget;
    auto layout_theme           = new QHBoxLayout(theme_container);
    auto theme_label            = new QLabel("颜色主题");
    auto theme_select           = new QComboBox;
    auto scheme_check_container = new QWidget;
    auto layout_scheme_check    = new QHBoxLayout(scheme_check_container);
    auto keep_scheme_check      = new QCheckBox("改变主题时保持色彩方案");
    auto color_picker_conatiner = new QFrame;
    auto layout_color_picker    = new QGridLayout(color_picker_conatiner);

    QList<QPair<QString, ColorScheme::ColorRole>> color_entries{
        {"背景",       ColorScheme::Window      },
        {"前景",       ColorScheme::WindowText  },
        {"边框",       ColorScheme::Border      },
        {"面板",       ColorScheme::Panel       },
        {"文本",       ColorScheme::Text        },
        {"文本背景", ColorScheme::TextBase    },
        {"选中文本", ColorScheme::SelectedText},
        {"悬停",       ColorScheme::Hover       },
        {"选中项",    ColorScheme::SelectedItem},
    };

    int index = 0;
    for (auto &[desc, color_role] : color_entries) {
        auto container        = new QWidget;
        auto layout_container = new QHBoxLayout(container);
        auto label            = new QLabel(desc);
        auto selector         = new widgetkit::ColorPreviewItem;

        layout_container->addWidget(selector);
        layout_container->addWidget(label);
        layout_color_picker->addWidget(container, index / 2, index % 2);

        selector->setFixedWidth(40);
        selector->setColor(schemes_[theme_][color_role]);

        connect(
            selector,
            &widgetkit::ColorPreviewItem::colorChanged,
            [this, role = color_role](QColor color) {
                schemes_[theme_][role] = color;
                emit schemeChanged(getTheme(), getScheme());
            });

        connect(
            this,
            &ColorSchemeDialog::schemeChanged,
            selector,
            [this, selector, role = color_role](ColorTheme theme, const ColorScheme &scheme) {
                blockSignals(true);
                selector->setColor(schemes_[theme_][role]);
                blockSignals(false);
            });

        ++index;
    }

    auto button_container = new QWidget;
    auto layout_button    = new QHBoxLayout(button_container);
    auto ok_button        = new QPushButton("确定");
    auto apply_button     = new QPushButton("应用");
    auto cancel_button    = new QPushButton("取消");

    layout_theme->addWidget(theme_label);
    layout_theme->addWidget(theme_select);
    layout_theme->addStretch();

    layout_scheme_check->addWidget(keep_scheme_check);

    layout_button->addStretch();
    layout_button->addWidget(ok_button);
    layout_button->addWidget(apply_button);
    layout_button->addWidget(cancel_button);

    layout->addWidget(theme_container);
    layout->addWidget(scheme_check_container);
    layout->addWidget(color_picker_conatiner);
    layout->addWidget(button_container);

    const auto margins = layout_theme->contentsMargins();
    layout_scheme_check->setContentsMargins(margins);
    layout_color_picker->setContentsMargins(margins);
    layout_button->setContentsMargins(margins);

    color_picker_conatiner->setWindowTitle("色彩方案");
    color_picker_conatiner->setFrameShape(QFrame::Box);
    color_picker_conatiner->setContentsMargins(2, 2, 2, 2);
    color_picker_conatiner->setFrameShadow(QFrame::Sunken);

    theme_select->addItem("亮色", QVariant::fromValue(ColorTheme::Light));
    theme_select->addItem("暗色", QVariant::fromValue(ColorTheme::Dark));
    theme_select->setCurrentIndex(theme_ == ColorTheme::Light ? 0 : 1);

    connect(ok_button, &QPushButton::clicked, this, &ColorSchemeDialog::accept);
    connect(apply_button, &QPushButton::clicked, this, [this] {
        emit applyRequested(getTheme(), getScheme());
    });
    connect(cancel_button, &QPushButton::clicked, this, &ColorSchemeDialog::reject);
    connect(
        theme_select,
        &QComboBox::currentIndexChanged,
        this,
        [this, keep_scheme_check, theme_select](int index) {
            const auto theme = theme_select->itemData(index).value<ColorTheme>();
            Q_ASSERT(theme_ != theme);
            if (const bool inherit_scheme = keep_scheme_check->isChecked()) {
                schemes_[theme] = schemes_[theme_];
            } else if (!schemes_.contains(theme)) {
                schemes_[theme] = ColorScheme::get_default_scheme_of_theme(theme);
            }
            theme_ = theme;
            emit schemeChanged(getTheme(), getScheme());
        });

    auto font = this->font();
    font.setPointSize(12);
    this->setFont(font);
}

} // namespace jwrite::ui
