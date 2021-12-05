#include <codecvt>
#include <locale>

#include "c2d_helper.h"
#include "colours.h"
#include "config.h"
#include "fs.h"
#include "gui.h"
#include "osk.h"
#include "textures.h"
#include "touch.h"
#include "utils.h"

static int row = 0, column = 0;
static bool copy = false, move = false, options_more = false;

namespace Options {
    static void ResetSelector(void) {
        row = 0;
        column = 0;
    }

    static void HandleMultipleCopy(MenuItem *item, Result (*func)()) {
        Result ret = 0;
        std::vector<FS_DirectoryEntry> entries;
        
        if (R_FAILED(ret = FS::GetDirList(item->checked_cwd.data(), entries)))
            return;
            
        for (u32 i = 0; i < item->checked_copy.size(); i++) {
            if (item->checked_copy.at(i)) {
                FS::Copy(&entries[i], item->checked_cwd);
                if (R_FAILED((*func)())) {
                    FS::GetDirList(cfg.cwd, item->entries);
                    GUI::ResetCheckbox(item);
                    break;
                }
            }
        }
        
        FS::GetDirList(cfg.cwd, item->entries);
        GUI::ResetCheckbox(item);
        entries.clear();
    }

    static void CreateFolder(MenuItem *item) {
        std::string path = cfg.cwd;
        std::string name = OSK::GetText("新しいフォルダ", "フォルダ名を入力");
        path.append(name);
        std::u16string path_u16 = std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t>{}.from_bytes(path.data());
        
        if (R_SUCCEEDED(FSUSER_CreateDirectory(archive, fsMakePath(PATH_UTF16, path_u16.c_str()), 0))) {
            FS::GetDirList(cfg.cwd, item->entries);
            GUI::ResetCheckbox(item);
        }
    }

    static void CreateFile(MenuItem *item) {
        std::string path = cfg.cwd;
        std::string name = OSK::GetText("新しいファイル", "ファイル名を入力");
        path.append(name);
        std::u16string path_u16 = std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t>{}.from_bytes(path.data());
        
        if (R_SUCCEEDED(FSUSER_CreateFile(archive, fsMakePath(PATH_UTF16, path_u16.c_str()), 0, 0))) {
            FS::GetDirList(cfg.cwd, item->entries);
            GUI::ResetCheckbox(item);
        }
    }

    static void Rename(MenuItem *item, const std::string &filename) {
        std::string path = OSK::GetText(filename, "新しい名前を入力");

        if (R_SUCCEEDED(FS::Rename(&item->entries[item->selected], path.c_str()))) {
            FS::GetDirList(cfg.cwd, item->entries);
            Options::ResetSelector();
            options_more = false;
            item->state = MENU_STATE_FILEBROWSER;
        }
    }

    static void Copy(MenuItem *item) {
        if (!copy) {
            if ((item->checked_count >= 1) && (item->checked_cwd.compare(cfg.cwd) != 0))
                GUI::ResetCheckbox(item);
            if (item->checked_count <= 1)
                FS::Copy(&item->entries[item->selected], cfg.cwd);
            
            copy = !copy;
            item->state = MENU_STATE_FILEBROWSER;
        }
        else {
            if ((item->checked_count > 1) && (item->checked_cwd.compare(cfg.cwd) != 0))
                Options::HandleMultipleCopy(item, &FS::Paste);
            else {
                if (R_SUCCEEDED(FS::Paste())) {
                    FS::GetDirList(cfg.cwd, item->entries);
                    GUI::ResetCheckbox(item);
                }
            }
            
            GUI::RecalcStorageSize(item);
            copy = !copy;
            item->state = MENU_STATE_FILEBROWSER;
        }
    }

    static void Move(MenuItem *item) {
        if (!move) {
            if ((item->checked_count >= 1) && (item->checked_cwd.compare(cfg.cwd) != 0))
                GUI::ResetCheckbox(item);
                
            if (item->checked_count <= 1)
                FS::Copy(&item->entries[item->selected], cfg.cwd);
        }
        else {
            if ((item->checked_count > 1) && (item->checked_cwd.compare(cfg.cwd) != 0))
                Options::HandleMultipleCopy(item, &FS::Move);
            else if (R_SUCCEEDED(FS::Move())) {
                FS::GetDirList(cfg.cwd, item->entries);
                GUI::ResetCheckbox(item);
            }
        }
        
        move = !move;
        item->state = MENU_STATE_FILEBROWSER;
    }
}

namespace GUI {
    static float cancel_width = 0.f, cancel_height = 0.f;

    void DisplayFileOptions(MenuItem *item) {
        C2D::Image(cfg.dark_theme? options_dialog_dark : options_dialog, 54, 30);
        C2D::Text(61, 34, 0.42f, cfg.dark_theme? TITLE_COLOUR_DARK : TITLE_COLOUR, "アクション");
        
        C2D::GetTextSize(0.42f, &cancel_width, &cancel_height, "キャンセル");
        
        if (row == 0 && column == 0)
            C2D::Rect(56, 69, 103, 36, cfg.dark_theme? SELECTOR_COLOUR_DARK : SELECTOR_COLOUR_LIGHT);
        else if (row == 1 && column == 0)
            C2D::Rect(160, 69, 103, 36, cfg.dark_theme? SELECTOR_COLOUR_DARK : SELECTOR_COLOUR_LIGHT);
        else if (row == 0 && column == 1)
            C2D::Rect(56, 105, 103, 36, cfg.dark_theme? SELECTOR_COLOUR_DARK : SELECTOR_COLOUR_LIGHT);
        else if (row == 1 && column == 1)
            C2D::Rect(160, 105, 103, 36, cfg.dark_theme? SELECTOR_COLOUR_DARK : SELECTOR_COLOUR_LIGHT);
        else if (row == 0 && column == 2 && !options_more)
            C2D::Rect(56, 142, 103, 36, cfg.dark_theme? SELECTOR_COLOUR_DARK : SELECTOR_COLOUR_LIGHT);
        else if (row == 1 && column == 2 && !options_more)
            C2D::Rect(160, 142, 103, 36, cfg.dark_theme? SELECTOR_COLOUR_DARK : SELECTOR_COLOUR_LIGHT);
        else if (column == 3 && !options_more)
            C2D::Rect((256 - cancel_width) - 5, (221 - cancel_height) - 5, cancel_width+ 10, cancel_height + 10, cfg.dark_theme? SELECTOR_COLOUR_DARK : SELECTOR_COLOUR_LIGHT);
        else if (column == 2 && options_more)
            C2D::Rect((256 - cancel_width) - 5, (221 - cancel_height) - 5, cancel_width + 10, cancel_height + 10, cfg.dark_theme? SELECTOR_COLOUR_DARK : SELECTOR_COLOUR_LIGHT);
            
        C2D::Text(256 - cancel_width, 221 - cancel_height - 3, 0.42f, cfg.dark_theme? TITLE_COLOUR_DARK : TITLE_COLOUR, "キャンセル");
        
        if (!options_more) {
            C2D::Text(66, 78, 0.42f, cfg.dark_theme? TEXT_MIN_COLOUR_DARK : TEXT_MIN_COLOUR_LIGHT, "プロパティ");
            C2D::Text(66, 114, 0.42f, cfg.dark_theme? TEXT_MIN_COLOUR_DARK : TEXT_MIN_COLOUR_LIGHT, copy? "貼り付け" : "コピー");
            C2D::Text(66, 150, 0.42f, cfg.dark_theme? TEXT_MIN_COLOUR_DARK : TEXT_MIN_COLOUR_LIGHT, "削除");
            C2D::Text(170, 78, 0.42f, cfg.dark_theme? TEXT_MIN_COLOUR_DARK : TEXT_MIN_COLOUR_LIGHT, "リフレッシュ");
            C2D::Text(170, 114, 0.42f, cfg.dark_theme? TEXT_MIN_COLOUR_DARK : TEXT_MIN_COLOUR_LIGHT, move? "貼り付け" : "移動");
            C2D::Text(170, 150, 0.42f, cfg.dark_theme? TEXT_MIN_COLOUR_DARK : TEXT_MIN_COLOUR_LIGHT, "次へ...");
        }
        else {
            C2D::Text(66, 78, 0.42f, cfg.dark_theme? TEXT_MIN_COLOUR_DARK : TEXT_MIN_COLOUR_LIGHT, "新しいフォルダ");
            C2D::Text(66, 114, 0.42f, cfg.dark_theme? TEXT_MIN_COLOUR_DARK : TEXT_MIN_COLOUR_LIGHT, "リネーム");
            C2D::Text(170, 78, 0.42f, cfg.dark_theme? TEXT_MIN_COLOUR_DARK : TEXT_MIN_COLOUR_LIGHT, "新しいファイル");
        }
    }

    void ControlFileOptions(MenuItem *item, u32 *kDown) {
        if (*kDown & KEY_RIGHT)
            row++;
        else if (*kDown & KEY_LEFT)
            row--;
        
        if (*kDown & KEY_DDOWN)
            column++;
        else if (*kDown & KEY_DUP)
            column--;

        if (!options_more) {
            Utils::SetBounds(&row, 0, 1);
            Utils::SetBounds(&column, 0, 3);
        }
        else {
            Utils::SetBounds(&column, 0, 2);
            
            if (column == 1)
                Utils::SetBounds(&row, 0, 0);
            else
                Utils::SetBounds(&row, 0, 1);
        }

        if (*kDown & KEY_A) {
            const std::u16string entry_name_utf16 = reinterpret_cast<const char16_t *>(item->entries[item->selected].name);
            const std::string filename = std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t>{}.to_bytes(entry_name_utf16.data());

            if (row == 0) {
                if (!options_more) {
                    if (column == 0)
                        item->state = MENU_STATE_PROPERTIES;
                    else if (column == 1)
                        Options::Copy(item);
                    else if (column == 2)
                        item->state = MENU_STATE_DELETE;
                }
                else {
                    if (column == 0)
                        Options::CreateFolder(item);
                    else if (column == 1)
                        Options::Rename(item, filename);
                }
            }
            else if (row == 1) {
                if (!options_more) {
                    if (column == 0) {
                        FS::GetDirList(cfg.cwd, item->entries);
                        Options::ResetSelector();
                        options_more = false;
                        item->selected = 0;
                        item->state = MENU_STATE_FILEBROWSER;
                    }
                    else if (column == 1)
                        Options::Move(item);
                    else if (column == 2) {
                        Options::ResetSelector();
                        options_more = true;
                    }
                }
                else {
                    if (column == 0)
                        Options::CreateFile(item);
                }
            }
            if (column == 3) {
                copy = false;
                move = false;
                Options::ResetSelector();
                options_more = false;
                item->state = MENU_STATE_FILEBROWSER;
            }
        }
        if (*kDown & KEY_B) {
            Options::ResetSelector();

            if (!options_more)
                item->state = MENU_STATE_FILEBROWSER;
            else
                options_more = false;
        }

        if (Touch::Rect(56, 69, 159, 104)) {
            row = 0;
            column = 0;
            
            if (*kDown & KEY_TOUCH) {
                if (!options_more)
                    item->state = MENU_STATE_PROPERTIES;
                else
                    Options::CreateFolder(item);
            }
        }
        else if (Touch::Rect(160, 69, 263, 104)) {
            row = 1;
            column = 0;
            
            if (*kDown & KEY_TOUCH) {
                if (!options_more) {
                    FS::GetDirList(cfg.cwd, item->entries);
                    Options::ResetSelector();
                    options_more = false;
                    item->selected = 0;
                    item->state = MENU_STATE_FILEBROWSER;
                }
                else
                    Options::CreateFile(item);
            }
        }
        else if (Touch::Rect(56, 105, 159, 141)) {
            row = 0;
            column = 1;
            
            if (*kDown & KEY_TOUCH) {
                if (!options_more)
                    Options::Copy(item);
                else {
                    const std::u16string entry_name_utf16 = reinterpret_cast<const char16_t *>(item->entries[item->selected].name);
                    const std::string filename = std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t>{}.to_bytes(entry_name_utf16.data());
                    Options::Rename(item, filename);
                }
            }
        }
        else if ((Touch::Rect(160, 105, 263, 141)) && (!options_more)) {
            row = 1;
            column = 1;
            
            if (*kDown & KEY_TOUCH)
                Options::Move(item);
        }
        else if ((Touch::Rect(56, 142, 159, 178)) && (!options_more)) {
            row = 0;
            column = 2;
            
            if (*kDown & KEY_TOUCH)
                item->state = MENU_STATE_DELETE;
        }
        else if ((Touch::Rect(160, 142, 263, 178)) && (!options_more)) {
            row = 1;
            column = 2;
            
            if (*kDown & KEY_TOUCH) {
                Options::ResetSelector();
                options_more = true;
            }
        }
        else if (Touch::Rect((256 - cancel_width) - 5, (221 - cancel_height) - 5, ((256 - cancel_width) - 5) + cancel_width + 10, 
            ((221 - cancel_height) - 5) + cancel_height + 10)) {
            if (options_more)
                column = 2;
            else
                column = 3;
                
            if (*kDown & KEY_TOUCH) {
                Options::ResetSelector();
                options_more = false;
                copy = false;
                move = false;
                item->state = MENU_STATE_FILEBROWSER;
            }
        }
    }
}
