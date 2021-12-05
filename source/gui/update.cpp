#include "c2d_helper.h"
#include "cia.h"
#include "colours.h"
#include "config.h"
#include "fs.h"
#include "gui.h"
#include "log.h"
#include "net.h"
#include "textures.h"
#include "touch.h"
#include "utils.h"

static float cancel_height = 0.f, cancel_width = 0.f, confirm_height = 0.f, confirm_width = 0.f, text_width = 0.f;

// Kinda messy, perhaps clean this up later if I ever revisit this.
namespace GUI {
    static int selection = 0;
    static const std::string error = "ネットワークに接続できませんでした。";
    static const std::string prompt = "ダウンロードしてインストールしますか vX.X.X?";
    static const std::string success = "アップデートがインストールされました。アプリケーションを再実行してください";
    static const std::string no_updates = "すでに最新バージョンを使用しています";
    static bool done = false;

    void DownloadHelper(const std::string &tag) {
        Net::Init();
        s32 prio = 0;
        download_progress = true;
        svcGetThreadPriority(&prio, CUR_THREAD_HANDLE);
        Thread thread = threadCreate(GUI::DownloadProgressBar, nullptr, 32 * 1024, prio - 1, -2, false);
        Net::GetLatestRelease(tag);
        download_progress = false;
        threadJoin(thread, U64_MAX);
        threadFree(thread);
        Net::Exit();
        
        if (envIsHomebrew()) {
            Result ret = 0;
            if (R_FAILED(ret = FSUSER_DeleteFile(sdmc_archive, fsMakePath(PATH_ASCII, __application_path__.c_str()))))
                Log::Error("FSUSER_DeleteFile(%s) failed: 0x%x\n", __application_path__, ret);
                
            if (R_FAILED(ret = FSUSER_RenameFile(sdmc_archive, fsMakePath(PATH_ASCII, "/3ds/3DShell/3DShell_UPDATE.3dsx"), sdmc_archive, fsMakePath(PATH_ASCII, __application_path__.c_str()))))
                Log::Error("FSUSER_RenameFile(update) failed: 0x%x\n", ret);
        }
        else
            CIA::InstallUpdate();
        
        done = true;
    }

    void DisplayUpdateOptions(bool *connection_status, bool *available, const std::string &tag) {
        C2D::Image(cfg.dark_theme? dialog_dark : dialog, ((320 - (dialog.subtex->width)) / 2), ((240 - (dialog.subtex->height)) / 2));

        if (!*connection_status) {
            C2D::Text(((320 - (dialog.subtex->width)) / 2) + 6, ((240 - (dialog.subtex->height)) / 2) + 6 - 3, 0.42f, cfg.dark_theme? TITLE_COLOUR_DARK : TITLE_COLOUR, "エラー");
            C2D::GetTextSize(0.42f, &text_width, nullptr, error.c_str());
            C2D::Text(((320 - (text_width)) / 2), ((240 - (dialog.subtex->height)) / 2) + 40 - 3, 0.42f, cfg.dark_theme? TEXT_MIN_COLOUR_DARK : TEXT_MIN_COLOUR_LIGHT, error.c_str());
        }
        else if ((*connection_status) && (*available) && (!tag.empty()) && (!done)) {
            C2D::Text(((320 - (dialog.subtex->width)) / 2) + 6, ((240 - (dialog.subtex->height)) / 2) + 6 - 3, 0.42f, cfg.dark_theme? TITLE_COLOUR_DARK : TITLE_COLOUR, "利用可能なアップデート");
            C2D::GetTextSize(0.42f, &text_width, nullptr, prompt.c_str());
            C2D::Textf(((320 - (text_width)) / 2), ((240 - (dialog.subtex->height)) / 2) + 40 - 3, 0.42f, cfg.dark_theme? TEXT_MIN_COLOUR_DARK : TEXT_MIN_COLOUR_LIGHT, 
                "ダウンロードしてインストールしますか %s?", tag.c_str());
        }
        else if (!*available) {
            C2D::Text(((320 - (dialog.subtex->width)) / 2) + 6, ((240 - (dialog.subtex->height)) / 2) + 6 - 3, 0.42f, cfg.dark_theme? TITLE_COLOUR_DARK : TITLE_COLOUR, "更新なし");
            C2D::GetTextSize(0.42f, &text_width, nullptr, no_updates.c_str());
            C2D::Text(((320 - (text_width)) / 2), ((240 - (dialog.subtex->height)) / 2) + 40 - 3, 0.42f, cfg.dark_theme? TEXT_MIN_COLOUR_DARK : TEXT_MIN_COLOUR_LIGHT, no_updates.c_str());
        }
        else if (done) {
            C2D::Text(((320 - (dialog.subtex->width)) / 2) + 6, ((240 - (dialog.subtex->height)) / 2) + 6 - 3, 0.42f, cfg.dark_theme? TITLE_COLOUR_DARK : TITLE_COLOUR, "更新に成功");
            C2D::GetTextSize(0.42f, &text_width, nullptr, success.c_str());
            C2D::Text(((320 - (text_width)) / 2), ((240 - (dialog.subtex->height)) / 2) + 40 - 3, 0.42f, cfg.dark_theme? TEXT_MIN_COLOUR_DARK : TEXT_MIN_COLOUR_LIGHT, success.c_str());
        }
        
        C2D::GetTextSize(0.42f, &confirm_width, &confirm_height, "はい");
        C2D::GetTextSize(0.42f, &cancel_width, &cancel_height, "いいえ");
        
        if (selection == 0)
            C2D::Rect((288 - cancel_width) - 5, (159 - cancel_height) - 5, cancel_width + 10, cancel_height + 10, cfg.dark_theme? SELECTOR_COLOUR_DARK : SELECTOR_COLOUR_LIGHT);
        else if (selection == 1)
            C2D::Rect((248 - (confirm_width)) - 5, (159 - confirm_height) - 5, confirm_width + 10, confirm_height + 10, cfg.dark_theme? SELECTOR_COLOUR_DARK : SELECTOR_COLOUR_LIGHT);
            
        if ((!done) && (*connection_status) && (*available))
            C2D::Text(248 - (confirm_width), (159 - confirm_height) - 3, 0.42f, cfg.dark_theme? TITLE_COLOUR_DARK : TITLE_COLOUR, "はい");
        
        C2D::Text(288 - cancel_width, (159 - cancel_height) - 3, 0.42f, cfg.dark_theme? TITLE_COLOUR_DARK : TITLE_COLOUR, ((!*connection_status) || (done) || (!*available))? "はい" : "いいえ");
    }

    void ControlUpdateOptions(MenuItem *item, u32 *kDown, bool *state, bool *connection_status, bool *available, const std::string &tag) {
        if (*kDown & KEY_RIGHT)
            selection++;
        else if (*kDown & KEY_LEFT)
            selection--;

        if (*kDown & KEY_A) {
            if (selection == 1)
                GUI::DownloadHelper(tag);
            else if (!done) {
                *state = false;
                item->state = MENU_STATE_SETTINGS;
            }
            else if (done)
                longjmp(exit_jmp, 1);

        }
        else if (*kDown & KEY_B) {
            *state = false;
            item->state = MENU_STATE_SETTINGS;
        }

        if (Touch::Rect((288 - cancel_width) - 5, (159 - cancel_height) - 5, ((288 - cancel_width) - 5) + cancel_width + 10, ((159 - cancel_height) - 5) + cancel_height + 10)) {
            selection = 0;
            
            if (*kDown & KEY_TOUCH) {
                if (!done) {
                    *state = false;
                    item->state = MENU_STATE_SETTINGS;
                }
                else if (done)
                    longjmp(exit_jmp, 1);
            }
        }
        else if (Touch::Rect((248 - (confirm_width)) - 5, (159 - confirm_height) - 5, ((248 - (confirm_width)) - 5) + confirm_width + 10, ((159 - confirm_height) - 5) + confirm_height + 10)) {
            selection = 1;
            
            if ((*kDown & KEY_TOUCH) && (!done) && (*connection_status) && (*available))
                GUI::DownloadHelper(tag);
        }

        if ((done) || (!*connection_status) || (!*available))
            Utils::SetBounds(&selection, 0, 0);
        else
            Utils::SetBounds(&selection, 0, 1);
    }
}
