#include "EpubBuilder.h"
#include "../minizip/zip.h"
#include <QDir>
#include <QStack>

namespace jwrite::epub {

static const char *DISPLAY_OPTIONS = R"(<?xml version="1.0" encoding="UTF-8"?>
<display_options>
  <platform name="*">
    <option name="specified-fonts">true</option>
  </platform>
</display_options>)";

static const char *CONTAINER = R"(<?xml version="1.0" encoding="UTF-8"?>
<container version="1.0" xmlns="urn:oasis:names:tc:opendocument:xmlns:container">
  <rootfiles>
    <rootfile full-path="EPUB/content.opf" media-type="application/oebps-package+xml" />
  </rootfiles>
</container>)";

static const char *CONTENT_TEMPLATE = R"(<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE html>
<html xmlns="http://www.w3.org/1999/xhtml" xmlns:epub="http://www.idpf.org/2007/ops" xml:lang="zh-CN">
<head>
  <meta charset="utf-8" />
  <meta name="generator" content="pandoc" />
  <title>ch001.xhtml</title>
  <style>
  </style>
  <link rel="stylesheet" type="text/css" href="../styles/stylesheet1.css" />
</head>
<body epub:type="bodymatter">
<section id="%1" class="level1">
<h1>%1</h1>
%2
</section>
</body>
</html>)";

static const char *TITLE_PAGE_TEMPLATE = R"(<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE html>
<html xmlns="http://www.w3.org/1999/xhtml" xmlns:epub="http://www.idpf.org/2007/ops" xml:lang="zh-CN">
<head>
  <meta charset="utf-8" />
  <meta name="generator" content="pandoc" />
  <title>%1</title>
  <style>
  </style>
  <link rel="stylesheet" type="text/css" href="../styles/stylesheet1.css" />
</head>
<body epub:type="frontmatter">
<section epub:type="titlepage" class="titlepage">
  <h1 class="title">%1</h1>
  <p class="author">%2</p>
</section>
</body>
</html>)";

static const char *CONTENT_OPF_TEMPLATE = R"(<?xml version="1.0" encoding="UTF-8"?>
<package version="3.0" xmlns="http://www.idpf.org/2007/opf" unique-identifier="epub-id-1" prefix="ibooks: http://vocabulary.itunes.apple.com/rdf/ibooks/vocabulary-extensions-1.0/">
  <metadata xmlns:dc="http://purl.org/dc/elements/1.1/" xmlns:opf="http://www.idpf.org/2007/opf">
    <dc:identifier id="epub-id-1">urn:uuid:c6d478d3-e5ad-4f3d-ba68-200fa4b04106</dc:identifier>
    <dc:title id="epub-title-1">%1</dc:title>
    <dc:date id="epub-date">2023-09-14T03:38:25Z</dc:date>
    <dc:language>zh-CN</dc:language>
    <dc:creator id="epub-creator-1">%2</dc:creator>
    <meta refines="#epub-creator-1" property="role" scheme="marc:relators">aut</meta>
    <meta property="dcterms:modified">2023-09-14T03:38:25Z</meta>
  </metadata>
  <manifest>
    <item id="ncx" href="toc.ncx" media-type="application/x-dtbncx+xml" />
    <item id="nav" href="nav.xhtml" media-type="application/xhtml+xml" properties="nav" />
    <item id="stylesheet1" href="styles/stylesheet1.css" media-type="text/css" />
    <item id="title_page_xhtml" href="text/title_page.xhtml" media-type="application/xhtml+xml" />%3
  </manifest>
  <spine toc="ncx">
    <itemref idref="title_page_xhtml" linear="yes" />
    <itemref idref="nav" />%4
  </spine>
  <guide>
    <reference type="toc" title="%1" href="nav.xhtml" />
  </guide>
</package>)";

static const char *NAV_TEMPLATE = R"(<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE html>
<html xmlns="http://www.w3.org/1999/xhtml" xmlns:epub="http://www.idpf.org/2007/ops" xml:lang="zh-CN">
<head>
    <meta charset="utf-8" />
    <meta name="generator" content="pandoc" />
    <title>%1</title>
    <style>
    </style>
    <link rel="stylesheet" type="text/css" href="styles/stylesheet1.css" />
</head>
<body epub:type="frontmatter">
    <nav epub:type="toc" id="toc">
        <h1 id="toc-title">%1</h1>
        <ol class="toc">%2
        </ol>
    </nav>
    <nav epub:type="landmarks" id="landmarks" hidden="hidden">
        <ol>
            <li>
                <a href="text/title_page.xhtml" epub:type="titlepage">封面</a>
            </li>
            <li>
                <a href="#toc" epub:type="toc">目录</a>
            </li>
        </ol>
    </nav>
</body>
</html>)";

static const char *NAV_TOP_ITEM_TEMPLATE = R"(
            <li id="toc-li-%1">
                <a href="text/ch%2.xhtml">%3</a>
                <ol class="toc">%4
                </ol>
            </li>)";

static const char *NAV_SUB_ITEM_TEMPLATE = R"(
                <li id="toc-li-%1">
                    <a href="text/ch%2.xhtml#%4">%3</a>
                </li>)";

static const char *TOC_NCX_TEMPLATE = R"(<?xml version="1.0" encoding="UTF-8"?>
<ncx version="2005-1" xmlns="http://www.daisy.org/z3986/2005/ncx/">
    <head>
        <meta name="dtb:uid" content="urn:uuid:c6d478d3-e5ad-4f3d-ba68-200fa4b04106" />
        <meta name="dtb:depth" content="1" />
        <meta name="dtb:totalPageCount" content="0" />
        <meta name="dtb:maxPageNumber" content="0" />
    </head>
    <docTitle>
        <text>%1</text>
    </docTitle>
    <navMap>
        <navPoint id="navPoint-0">
            <navLabel>
                <text>%1</text>
            </navLabel>
            <content src="text/title_page.xhtml" />
        </navPoint>%2
    </navMap>
</ncx>)";

static const char *TOC_NCX_TOP_ITEM_TEMPLATE = R"(
        <navPoint id="navPoint-%1">
            <navLabel>
                <text>%3</text>
            </navLabel>
            <content src="text/ch%2.xhtml" />%4
        </navPoint>)";

static const char *TOC_NCX_SUB_ITEM_TEMPLATE = R"(
            <navPoint id="navPoint-%1">
                <navLabel>
                    <text>%3</text>
                </navLabel>
                <content src="text/ch%2.xhtml#%4" />
            </navPoint>)";

EpubBuilder::EpubBuilder(const QString &filename)
    : filename_(filename)
    , book_title_("未命名书籍")
    , author_("佚名") {
    Q_ASSERT(build_dir_.isValid());
}

void EpubBuilder::build() {
    write_meta_inf();
    write_content_opf();
    write_stylesheet();
    write_toc_ncx();
    write_nav_html();
    write_title_page();

    auto zip = zipOpen(filename_.toLocal8Bit().data(), false);

    const QDir root(build_dir_.path());

    QStack<QDir> stack;
    stack.push(root);

    while (!stack.empty()) {
        auto dir = stack.pop();
        for (auto entry : dir.entryInfoList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot)) {
            if (entry.isDir()) {
                stack.push(entry.filePath());
                continue;
            }

            if (!entry.isFile()) { continue; }

            QFile file(entry.filePath());
            if (!file.open(QIODevice::ReadOnly)) { continue; }

            zip_fileinfo zipfi{};
            zipOpenNewFileInZip(
                zip,
                root.relativeFilePath(entry.filePath()).toStdString().c_str(),
                &zipfi,
                nullptr,
                0,
                nullptr,
                0,
                nullptr,
                Z_DEFLATED,
                Z_DEFAULT_COMPRESSION);

            const auto content = file.readAll();
            zipWriteInFileInZip(zip, content.data(), content.size());
        }
    }

    zipCloseFileInZip(zip);
    zipClose(zip, nullptr);
}

EpubBuilder &EpubBuilder::feed(FeedCallback request) {
    QDir dir(build_dir_.path());

    if (!dir.exists("EPUB")) { dir.mkdir("EPUB"); }
    dir.cd("EPUB");
    if (!dir.exists("text")) { dir.mkdir("text"); }
    dir.cd("text");

    for (int vol_index = 0; vol_index < toc_marker_.size(); ++vol_index) {
        QStringList chap_titles;
        QString     content;

        for (int chap_index = 0; chap_index < toc_marker_[vol_index]; ++chap_index) {
            QString title{};
            QString chapter{};
            request(vol_index, chap_index, title, chapter);
            chap_titles << title;

            content += QString("<section id=\"%1\" class=\"level2\">\n<h2>%1</h2>\n").arg(title);
            for (const auto &para : chapter.split('\n')) {
                content += QString("<p>%1</p>\n").arg(para);
            }
            content += "</section>\n";
        }

        chap_toc_ << chap_titles;
        content = QString(CONTENT_TEMPLATE).arg(vol_toc_[vol_index]).arg(content);

        QFile file(dir.filePath(QString("ch%1.xhtml").arg(vol_index + 1)));
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) { continue; }

        file.write(content.toUtf8());
    }

    return *this;
}

void EpubBuilder::write_title_page() {
    QDir dir(build_dir_.path());

    if (!dir.exists("EPUB")) { dir.mkdir("EPUB"); }
    dir.cd("EPUB");
    if (!dir.exists("text")) { dir.mkdir("text"); }
    dir.cd("text");

    QFile file(dir.filePath("title_page.xhtml"));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) { return; }

    file.write(QString(TITLE_PAGE_TEMPLATE).arg(book_title_).arg(author_).toUtf8());
}

void EpubBuilder::write_stylesheet() {
    QDir dir(build_dir_.path());

    if (!dir.exists("EPUB")) { dir.mkdir("EPUB"); }
    dir.cd("EPUB");
    if (!dir.exists("styles")) { dir.mkdir("styles"); }
    dir.cd("styles");

    QFile stylesheet(":/res/template/epub/style.css");
    if (!stylesheet.open(QIODevice::ReadOnly | QIODevice::Text)) { return; }

    QFile file(dir.filePath("stylesheet1.css"));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) { return; }

    file.write(stylesheet.readAll());
}

void EpubBuilder::write_toc_ncx() {
    QDir dir(build_dir_.path());

    if (!dir.exists("EPUB")) { dir.mkdir("EPUB"); }
    dir.cd("EPUB");

    QString toc_items;
    int     nav_point = 0;
    for (int i = 0; i < toc_marker_.size(); ++i) {
        const int nav_index = ++nav_point;
        QString   vol_toc_items;
        for (int j = 0; j < toc_marker_[i]; ++j) {
            const int nav_index = ++nav_point;
            auto      title     = chap_toc_[i][j];
            vol_toc_items +=
                QString(TOC_NCX_SUB_ITEM_TEMPLATE).arg(nav_index).arg(i + 1).arg(title).arg(title);
        }
        toc_items += QString(TOC_NCX_TOP_ITEM_TEMPLATE)
                         .arg(nav_index)
                         .arg(i + 1)
                         .arg(vol_toc_[i])
                         .arg(vol_toc_items);
    }

    const auto content = QString(TOC_NCX_TEMPLATE).arg(book_title_).arg(toc_items);

    QFile file(dir.filePath("toc.ncx"));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) { return; }

    file.write(content.toUtf8());
}

void EpubBuilder::write_nav_html() {
    QDir dir(build_dir_.path());

    if (!dir.exists("EPUB")) { dir.mkdir("EPUB"); }
    dir.cd("EPUB");

    QString nav_items;
    int     index = 0;
    for (int i = 0; i < toc_marker_.size(); ++i) {
        const int vol_li_index = ++index;
        QString   nav_vol_items;
        for (int j = 0; j < toc_marker_[i]; ++j) {
            const int chap_li_index = ++index;
            auto      title         = chap_toc_[i][j];
            nav_vol_items +=
                QString(NAV_SUB_ITEM_TEMPLATE).arg(chap_li_index).arg(i + 1).arg(title).arg(title);
        }
        nav_items += QString(NAV_TOP_ITEM_TEMPLATE)
                         .arg(vol_li_index)
                         .arg(i + 1)
                         .arg(vol_toc_[i])
                         .arg(nav_vol_items);
    }

    const auto content = QString(NAV_TEMPLATE).arg(book_title_).arg(nav_items);

    QFile file(dir.filePath("nav.xhtml"));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) { return; }

    file.write(content.toUtf8());
}

void EpubBuilder::write_content_opf() {
    QDir dir(build_dir_.path());

    if (!dir.exists("EPUB")) { dir.mkdir("EPUB"); }
    dir.cd("EPUB");

    QString toc_ref;
    QString toc;

    const QString toc_ref_template("\n    <item id=\"ch%1_xhtml\" href=\"text/ch%1.xhtml\" "
                                   "media-type=\"application/xhtml+xml\" />");
    const QString toc_template("\n    <itemref idref=\"ch%1_xhtml\" />");

    for (int i = 0; i < toc_marker_.size(); ++i) {
        toc_ref += toc_ref_template.arg(i + 1);
        toc     += toc_template.arg(i + 1);
    }

    const auto content =
        QString(CONTENT_OPF_TEMPLATE).arg(book_title_).arg(author_).arg(toc_ref).arg(toc);

    QFile file(dir.filePath("content.opf"));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) { return; }

    file.write(content.toUtf8());
}

void EpubBuilder::write_meta_inf() {
    QDir dir(build_dir_.path());

    if (!dir.exists("META-INF")) { dir.mkdir("META-INF"); }

    if (QFile file(dir.filePath("mimetype")); file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        file.write("application/epub+zip");
    }

    dir.cd("META-INF");

    if (QFile file(dir.filePath("com.apple.ibooks.display-options.xml"));
        file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        file.write(DISPLAY_OPTIONS);
    }

    if (QFile file(dir.filePath("container.xml"));
        file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        file.write(CONTAINER);
    }
}

} // namespace jwrite::epub
