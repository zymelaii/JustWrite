#include <jwrite/InputMethodHook.h>
#include <QDebug>

#ifdef WIN32
#include <windows.h>
#endif

namespace jwrite {

bool InputMethodHook::nativeEventFilter(
    const QByteArray &eventType, void *message, qintptr *result) {
#ifdef WIN32
#if 0
    auto msg = reinterpret_cast<MSG *>(message);
    switch (msg->message) {
        case WM_IME_STARTCOMPOSITION: {
        } break;
        case WM_IME_ENDCOMPOSITION: {
        } break;
        case WM_IME_COMPOSITION: {
        } break;
        case WM_IME_COMPOSITIONFULL: {
        } break;
        case WM_IME_CONTROL: {
            if (msg->wParam == IMC_GETCOMPOSITIONWINDOW) {
                auto comp = reinterpret_cast<COMPOSITIONFORM *>(msg->lParam);
                qDebug() << "IMC_GETCOMPOSITIONWINDOW" << comp->ptCurrentPos.x
                         << comp->ptCurrentPos.y;
            }
        } break;
        case WM_IME_NOTIFY: {
            QMap<int, QString> TABLE{
                {IMN_CLOSESTATUSWINDOW,    "IMN_CLOSESTATUSWINDOW"   },
                {IMN_OPENSTATUSWINDOW,     "IMN_OPENSTATUSWINDOW"    },
                {IMN_CHANGECANDIDATE,      "IMN_CHANGECANDIDATE"     },
                {IMN_CLOSECANDIDATE,       "IMN_CLOSECANDIDATE"      },
                {IMN_OPENCANDIDATE,        "IMN_OPENCANDIDATE"       },
                {IMN_SETCONVERSIONMODE,    "IMN_SETCONVERSIONMODE"   },
                {IMN_SETSENTENCEMODE,      "IMN_SETSENTENCEMODE"     },
                {IMN_SETOPENSTATUS,        "IMN_SETOPENSTATUS"       },
                {IMN_SETCANDIDATEPOS,      "IMN_SETCANDIDATEPOS"     },
                {IMN_SETCOMPOSITIONFONT,   "IMN_SETCOMPOSITIONFONT"  },
                {IMN_SETCOMPOSITIONWINDOW, "IMN_SETCOMPOSITIONWINDOW"},
                {IMN_SETSTATUSWINDOWPOS,   "IMN_SETSTATUSWINDOWPOS"  },
                {IMN_GUIDELINE,            "IMN_GUIDELINE"           },
                {IMN_PRIVATE,              "IMN_PRIVATE"             },
            };
            if (TABLE.contains(msg->wParam)) { qDebug().noquote() << TABLE[msg->wParam]; }
        } break;
        case WM_IME_REQUEST: {
        } break;
        case WM_IME_SELECT: {
        } break;
        case WM_IME_SETCONTEXT: {
        } break;
    }
#endif
#endif
    return false;
}

} // namespace jwrite
