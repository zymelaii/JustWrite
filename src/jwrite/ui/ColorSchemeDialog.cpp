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
    init();
}

void ColorSchemeDialog::init() {
    auto layout = new QVBoxLayout(this);

    auto theme_container        = new QWidget;
    auto layout_theme           = new QHBoxLayout(theme_container);
    auto theme_label            = new QLabel(tr("ColorSchemeDialog.theme_label.text"));
    auto theme_select           = new QComboBox;
    auto scheme_check_container = new QWidget;
    auto layout_scheme_check    = new QHBoxLayout(scheme_check_container);
    auto keep_scheme_check      = new QCheckBox(tr("ColorSchemeDialog.keep_scheme_check.text"));
    auto color_picker_conatiner = new QFrame;
    auto layout_color_picker    = new QGridLayout(color_picker_conatiner);

    QList<QPair<QString, ColorScheme::ColorRole>> color_entries{
        {tr("ColorSchemeDialog.color_entry.window"),               ColorScheme::Window            },
        {tr("ColorSchemeDialog.color_entry.window_text"),          ColorScheme::WindowText        },
        {tr("ColorSchemeDialog.color_entry.border"),               ColorScheme::Border            },
        {tr("ColorSchemeDialog.color_entry.panel"),                ColorScheme::Panel             },
        {tr("ColorSchemeDialog.color_entry.text"),                 ColorScheme::Text              },
        {tr("ColorSchemeDialog.color_entry.text_base"),            ColorScheme::TextBase          },
        {tr("ColorSchemeDialog.color_entry.selected_text"),        ColorScheme::SelectedText      },
        {tr("ColorSchemeDialog.color_entry.hover"),                ColorScheme::Hover             },
        {tr("ColorSchemeDialog.color_entry.selected_item"),        ColorScheme::SelectedItem      },
        {tr("ColorSchemeDialog.color_entry.floating_item"),        ColorScheme::FloatingItem      },
        {tr("ColorSchemeDialog.color_entry.floating_item_border"), ColorScheme::FloatingItemBorder},
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
                emit on_scheme_change(theme(), scheme());
            });

        connect(
            this,
            &ColorSchemeDialog::on_scheme_change,
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
    auto ok_button        = new QPushButton(tr("ColorSchemeDialog.ok_button.text"));
    auto apply_button     = new QPushButton(tr("ColorSchemeDialog.apply_button.text"));
    auto cancel_button    = new QPushButton(tr("ColorSchemeDialog.cancel_button.text"));

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

    color_picker_conatiner->setWindowTitle(tr("ColorSchemeDialog.color_picker.caption"));
    color_picker_conatiner->setFrameShape(QFrame::Box);
    color_picker_conatiner->setContentsMargins(2, 2, 2, 2);
    color_picker_conatiner->setFrameShadow(QFrame::Sunken);

    theme_select->addItem(
        tr("ColorSchemeDialog.theme_select.light"), QVariant::fromValue(ColorTheme::Light));
    theme_select->addItem(
        tr("ColorSchemeDialog.theme_select.dark"), QVariant::fromValue(ColorTheme::Dark));
    theme_select->setCurrentIndex(theme_ == ColorTheme::Light ? 0 : 1);

    connect(ok_button, &QPushButton::clicked, this, &ColorSchemeDialog::accept);
    connect(apply_button, &QPushButton::clicked, this, [this] {
        emit on_request_apply(theme(), scheme());
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
            emit on_scheme_change(this->theme(), this->scheme());
        });

    auto font = this->font();
    font.setPointSize(12);
    this->setFont(font);

    updateGeometry();
}

} // namespace jwrite::ui
