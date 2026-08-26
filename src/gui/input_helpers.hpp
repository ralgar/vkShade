#pragma once

#include <imgui.h>

#include "../input/key_codes.hpp"

namespace vkShade
{
    inline ImGuiKey to_imgui_key(KeyCode key)
    {
        switch (key)
        {
            case KeyCode::KEY_0:             return ImGuiKey_0;
            case KeyCode::KEY_1:             return ImGuiKey_1;
            case KeyCode::KEY_2:             return ImGuiKey_2;
            case KeyCode::KEY_3:             return ImGuiKey_3;
            case KeyCode::KEY_4:             return ImGuiKey_4;
            case KeyCode::KEY_5:             return ImGuiKey_5;
            case KeyCode::KEY_6:             return ImGuiKey_6;
            case KeyCode::KEY_7:             return ImGuiKey_7;
            case KeyCode::KEY_8:             return ImGuiKey_8;
            case KeyCode::KEY_9:             return ImGuiKey_9;

            case KeyCode::KEY_A:             return ImGuiKey_A;
            case KeyCode::KEY_B:             return ImGuiKey_B;
            case KeyCode::KEY_C:             return ImGuiKey_C;
            case KeyCode::KEY_D:             return ImGuiKey_D;
            case KeyCode::KEY_E:             return ImGuiKey_E;
            case KeyCode::KEY_F:             return ImGuiKey_F;
            case KeyCode::KEY_G:             return ImGuiKey_G;
            case KeyCode::KEY_H:             return ImGuiKey_H;
            case KeyCode::KEY_I:             return ImGuiKey_I;
            case KeyCode::KEY_J:             return ImGuiKey_J;
            case KeyCode::KEY_K:             return ImGuiKey_K;
            case KeyCode::KEY_L:             return ImGuiKey_L;
            case KeyCode::KEY_M:             return ImGuiKey_M;
            case KeyCode::KEY_N:             return ImGuiKey_N;
            case KeyCode::KEY_O:             return ImGuiKey_O;
            case KeyCode::KEY_P:             return ImGuiKey_P;
            case KeyCode::KEY_Q:             return ImGuiKey_Q;
            case KeyCode::KEY_R:             return ImGuiKey_R;
            case KeyCode::KEY_S:             return ImGuiKey_S;
            case KeyCode::KEY_T:             return ImGuiKey_T;
            case KeyCode::KEY_U:             return ImGuiKey_U;
            case KeyCode::KEY_V:             return ImGuiKey_V;
            case KeyCode::KEY_W:             return ImGuiKey_W;
            case KeyCode::KEY_X:             return ImGuiKey_X;
            case KeyCode::KEY_Y:             return ImGuiKey_Y;
            case KeyCode::KEY_Z:             return ImGuiKey_Z;

            case KeyCode::KEY_BACKQUOTE:     return ImGuiKey_GraveAccent;
            case KeyCode::KEY_TAB:           return ImGuiKey_Tab;
            case KeyCode::KEY_LSHIFT:        return ImGuiKey_LeftShift;
            case KeyCode::KEY_RSHIFT:        return ImGuiKey_RightShift;
            case KeyCode::KEY_LCTRL:         return ImGuiKey_LeftCtrl;
            case KeyCode::KEY_RCTRL:         return ImGuiKey_RightCtrl;
            case KeyCode::KEY_LALT:          return ImGuiKey_LeftAlt;
            case KeyCode::KEY_RALT:          return ImGuiKey_RightAlt;
            case KeyCode::KEY_SPACE:         return ImGuiKey_Space;
            case KeyCode::KEY_ESCAPE:        return ImGuiKey_Escape;
            case KeyCode::KEY_BACKSPACE:     return ImGuiKey_Backspace;
            case KeyCode::KEY_RETURN:        return ImGuiKey_Enter;
            case KeyCode::KEY_EQUALS:        return ImGuiKey_Equal;
            case KeyCode::KEY_MINUS:         return ImGuiKey_Minus;
            case KeyCode::KEY_LEFTBRACKET:   return ImGuiKey_LeftBracket;
            case KeyCode::KEY_RIGHTBRACKET:  return ImGuiKey_RightBracket;
            case KeyCode::KEY_BACKSLASH:     return ImGuiKey_Backslash;
            case KeyCode::KEY_SEMICOLON:     return ImGuiKey_Semicolon;
            case KeyCode::KEY_QUOTE:         return ImGuiKey_Apostrophe;
            case KeyCode::KEY_SLASH:         return ImGuiKey_Slash;
            case KeyCode::KEY_COMMA:         return ImGuiKey_Comma;
            case KeyCode::KEY_PERIOD:        return ImGuiKey_Period;

            case KeyCode::KEY_INSERT:        return ImGuiKey_Insert;
            case KeyCode::KEY_DELETE:        return ImGuiKey_Delete;
            case KeyCode::KEY_HOME:          return ImGuiKey_Home;
            case KeyCode::KEY_END:           return ImGuiKey_End;
            case KeyCode::KEY_PAGEUP:        return ImGuiKey_PageUp;
            case KeyCode::KEY_PAGEDOWN:      return ImGuiKey_PageDown;
            case KeyCode::KEY_PRINTSCREEN:   return ImGuiKey_PrintScreen;
            case KeyCode::KEY_UP:            return ImGuiKey_UpArrow;
            case KeyCode::KEY_DOWN:          return ImGuiKey_DownArrow;
            case KeyCode::KEY_LEFT:          return ImGuiKey_LeftArrow;
            case KeyCode::KEY_RIGHT:         return ImGuiKey_RightArrow;

            case KeyCode::KEY_NUMPAD_PLUS:   return ImGuiKey_KeypadAdd;
            case KeyCode::KEY_NUMPAD_MINUS:  return ImGuiKey_KeypadSubtract;
            case KeyCode::KEY_NUMPAD_MULTIPLY:return ImGuiKey_KeypadMultiply;
            case KeyCode::KEY_NUMPAD_DIVIDE: return ImGuiKey_KeypadDivide;
            case KeyCode::KEY_NUMPAD_PERIOD: return ImGuiKey_KeypadDecimal;
            case KeyCode::KEY_NUMPAD_ENTER:  return ImGuiKey_KeypadEnter;
            case KeyCode::KEY_NUMPAD_0:      return ImGuiKey_Keypad0;
            case KeyCode::KEY_NUMPAD_1:      return ImGuiKey_Keypad1;
            case KeyCode::KEY_NUMPAD_2:      return ImGuiKey_Keypad2;
            case KeyCode::KEY_NUMPAD_3:      return ImGuiKey_Keypad3;
            case KeyCode::KEY_NUMPAD_4:      return ImGuiKey_Keypad4;
            case KeyCode::KEY_NUMPAD_5:      return ImGuiKey_Keypad5;
            case KeyCode::KEY_NUMPAD_6:      return ImGuiKey_Keypad6;
            case KeyCode::KEY_NUMPAD_7:      return ImGuiKey_Keypad7;
            case KeyCode::KEY_NUMPAD_8:      return ImGuiKey_Keypad8;
            case KeyCode::KEY_NUMPAD_9:      return ImGuiKey_Keypad9;

            case KeyCode::KEY_F1:            return ImGuiKey_F1;
            case KeyCode::KEY_F2:            return ImGuiKey_F2;
            case KeyCode::KEY_F3:            return ImGuiKey_F3;
            case KeyCode::KEY_F4:            return ImGuiKey_F4;
            case KeyCode::KEY_F5:            return ImGuiKey_F5;
            case KeyCode::KEY_F6:            return ImGuiKey_F6;
            case KeyCode::KEY_F7:            return ImGuiKey_F7;
            case KeyCode::KEY_F8:            return ImGuiKey_F8;
            case KeyCode::KEY_F9:            return ImGuiKey_F9;
            case KeyCode::KEY_F10:           return ImGuiKey_F10;
            case KeyCode::KEY_F11:           return ImGuiKey_F11;
            case KeyCode::KEY_F12:           return ImGuiKey_F12;

            default:                         return ImGuiKey_None;
        }
    }
}
