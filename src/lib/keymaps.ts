import { platform } from "@tauri-apps/plugin-os";
import { MouseLeftClickIcon, MouseMiddleClickIcon, MouseRightClickIcon, MouseRightDragIcon, MouseScrollDownIcon, MouseScrollUpIcon, ReturnIcon } from "@/components/ui/icons";
import { ArrowBigUpDashIcon, ArrowBigUpIcon, ArrowDownIcon, ArrowDownToLineIcon, ArrowLeftIcon, ArrowLeftRightIcon, ArrowRightIcon, ArrowRightToLineIcon, ArrowUpIcon, ArrowUpToLineIcon, ChevronUpIcon, CircleArrowOutUpLeftIcon, CommandIcon, DeleteIcon, Grid2X2Icon, ImageIcon, LockIcon, LucideIcon, MouseIcon, MoveDownRightIcon, MoveUpLeftIcon, OptionIcon, PauseIcon, SpaceIcon, SparkleIcon, Volume2Icon, VolumeXIcon } from "lucide-react";

// ───────────── Platform Logic ─────────────
const currentPlatform = platform();

interface SwitchPlatformConfig<T> {
    windows: T;
    macos: T;
    linux?: T;
}

function switchPlatform<T>(config: SwitchPlatformConfig<T>): T {
    if (currentPlatform === 'macos') {
        return config.macos;
    } else if (currentPlatform === 'linux' && config.linux !== undefined) {
        return config.linux!;
    }
    return config.windows;
}

// ───────────── Key mapping ─────────────
export interface DisplayData {
    // textual representation
    label: string;
    // short label if any like ctrl for control
    shortLabel?: string;
    // glyph representation if any like ⌃ for control
    glyph?: string;
    // secondary symbol if any like @ for digit 2
    symbol?: string;
    // icon path if can be represented with iconography
    icon?: LucideIcon;
    // category
    category?: "modifier" | "letter" | "digit" | "punctuation" | "function" | "arrow" | "navigation" | "special" | "numpad" | "mouse";
}

// We use the string names from the provided Rust enum as keys
export const keymaps: Record<string, DisplayData> = {
    // ───────────── Function keys ─────────────
    F1: { label: "F1", category: "function" },
    F2: { label: "F2", category: "function" },
    F3: { label: "F3", category: "function" },
    F4: { label: "F4", category: "function" },
    F5: { label: "F5", category: "function" },
    F6: { label: "F6", category: "function" },
    F7: { label: "F7", category: "function" },
    F8: { label: "F8", category: "function" },
    F9: { label: "F9", category: "function" },
    F10: { label: "F10", category: "function" },
    F11: { label: "F11", category: "function" },
    F12: { label: "F12", category: "function" },
    // ───────────── Navigation ─────────────
    PrintScreen: {
        label: "打印屏幕",
        shortLabel: "截屏",
        icon: ImageIcon,
    },
    Pause: {
        label: "暂停",
        shortLabel: "暂停",
        icon: PauseIcon,
    },
    Backspace: {
        label: switchPlatform({
            windows: "退格",
            macos: "删除",
        }),
        shortLabel: switchPlatform({
            windows: "退格",
            macos: "删除",
        }),
        glyph: "⌫",
        icon: DeleteIcon,
        category: "special",
    },
    Tab: {
        label: "制表",
        glyph: "⇆",
        icon: ArrowLeftRightIcon,
        category: "special",
    },
    Space: {
        label: "空格",
        glyph: "⎵",
        icon: SpaceIcon,
    },
    Return: {
        label: switchPlatform({
            windows: "回车",
            macos: "回车",
        }),
        glyph: "↩",
        icon: ReturnIcon,
        category: "special",
    },
    Apps: {
        label: "菜单",
        glyph: "☰",
    },
    Insert: {
        label: "插入",
        shortLabel: "插入",
        glyph: "⇥",
        icon: ArrowRightToLineIcon,
        category: "special",
    },
    Delete: {
        label: "删除",
        shortLabel: "删除",
        glyph: "⌦",
        icon: DeleteIcon,
        category: "special",
    },
    Home: {
        label: "主页",
        glyph: "⇱",
        icon: MoveUpLeftIcon,
        category: "navigation",
    },
    End: {
        label: "末尾",
        glyph: "⇲",
        icon: MoveDownRightIcon,
        category: "navigation",
    },
    PageUp: {
        label: "向上翻页",
        shortLabel: "上翻页",
        glyph: "⤒",
        icon: ArrowUpToLineIcon,
        category: "navigation",
    },
    PageDown: {
        label: "向下翻页",
        shortLabel: "下翻页",
        glyph: "⤓",
        icon: ArrowDownToLineIcon,
        category: "navigation",
    },
    UpArrow: {
        label: "上",
        glyph: "↑",
        icon: ArrowUpIcon,
        category: "arrow",
    },
    DownArrow: {
        label: "下",
        glyph: "↓",
        icon: ArrowDownIcon,
        category: "arrow",
    },
    LeftArrow: {
        label: "左",
        glyph: "←",
        icon: ArrowLeftIcon,
        category: "arrow",
    },
    RightArrow: {
        label: "右",
        glyph: "→",
        icon: ArrowRightIcon,
        category: "arrow",
    },
    CapsLock: {
        label: "大写锁定",
        glyph: "⇪",
        icon: ArrowBigUpDashIcon,
    },
    ScrollLock: {
        label: "滚动锁定",
        glyph: "🖱",
        icon: MouseIcon,
    },
    NumLock: {
        label: "数字锁定",
        icon: LockIcon,
    },
    Escape: {
        label: "退出",
        shortLabel: "Esc",
        glyph: "⎋",
        icon: CircleArrowOutUpLeftIcon,
        category: "special",
    },

    // ───────────── Digits ──────────────
    Num1: {
        label: "1",
        symbol: "!",
        category: "digit",
    },
    Num2: {
        label: "2",
        symbol: "@",
        category: "digit",
    },
    Num3: {
        label: "3",
        symbol: "#",
        category: "digit",
    },
    Num4: {
        label: "4",
        symbol: "$",
        category: "digit",
    },
    Num5: {
        label: "5",
        symbol: "%",
        category: "digit",
    },
    Num6: {
        label: "6",
        symbol: "^",
        category: "digit",
    },
    Num7: {
        label: "7",
        symbol: "&",
        category: "digit",
    },
    Num8: {
        label: "8",
        symbol: "*",
        category: "digit",
    },
    Num9: {
        label: "9",
        symbol: "(",
        category: "digit",
    },
    Num0: {
        label: "0",
        symbol: ")",
        category: "digit",
    },
    // ───────────── Letters ─────────────
    KeyA: { label: "A", category: "letter" },
    KeyB: { label: "B", category: "letter" },
    KeyC: { label: "C", category: "letter" },
    KeyD: { label: "D", category: "letter" },
    KeyE: { label: "E", category: "letter" },
    KeyF: { label: "F", category: "letter" },
    KeyG: { label: "G", category: "letter" },
    KeyH: { label: "H", category: "letter" },
    KeyI: { label: "I", category: "letter" },
    KeyJ: { label: "J", category: "letter" },
    KeyK: { label: "K", category: "letter" },
    KeyL: { label: "L", category: "letter" },
    KeyM: { label: "M", category: "letter" },
    KeyN: { label: "N", category: "letter" },
    KeyO: { label: "O", category: "letter" },
    KeyP: { label: "P", category: "letter" },
    KeyQ: { label: "Q", category: "letter" },
    KeyR: { label: "R", category: "letter" },
    KeyS: { label: "S", category: "letter" },
    KeyT: { label: "T", category: "letter" },
    KeyU: { label: "U", category: "letter" },
    KeyV: { label: "V", category: "letter" },
    KeyW: { label: "W", category: "letter" },
    KeyX: { label: "X", category: "letter" },
    KeyY: { label: "Y", category: "letter" },
    KeyZ: { label: "Z", category: "letter" },
    // ───────────── Punctuation ─────────────
    BackQuote: {
        label: "`",
        symbol: "~",
        category: "punctuation",
    },
    Minus: {
        label: "-",
        symbol: "_",
        category: "punctuation",
    },
    Equal: {
        label: "=",
        symbol: "+",
        category: "punctuation",
    },
    LeftBracket: {
        label: "[",
        symbol: "{",
        category: "punctuation",
    },
    RightBracket: {
        label: "]",
        symbol: "}",
        category: "punctuation",
    },
    BackSlash: {
        label: "\\",
        symbol: "|",
        category: "punctuation",
    },
    SemiColon: {
        label: ";",
        symbol: ":",
        category: "punctuation",
    },
    Quote: {
        label: "'",
        symbol: "\"",
        category: "punctuation",
    },
    Comma: {
        label: ",",
        symbol: "<",
        category: "punctuation",
    },
    Dot: {
        label: ".",
        symbol: ">",
        category: "punctuation",
    },
    Slash: {
        label: "?",
        symbol: "/",
        category: "punctuation",
    },
    // ───────────── Numpad ─────────────
    KpDivide: { label: "/", category: "punctuation" },
    KpMultiply: { label: "*", category: "punctuation" },
    KpMinus: { label: "-", category: "punctuation" },
    KpPlus: { label: "+", category: "punctuation" },
    KpEqual: { label: "=", category: "punctuation" },
    KpComma: { label: ",", category: "punctuation" },
    KpReturn: {
        label: "回车",
        glyph: "↩",
        category: "numpad",
    },
    KpDecimal: {
        label: ".",
        symbol: "删除",
        category: "numpad",
    },
    Kp0: {
        label: "0",
        symbol: "插入",
        category: "numpad",
    },
    Kp1: {
        label: "1",
        symbol: "末尾",
        category: "numpad",
    },
    Kp2: {
        label: "2",
        symbol: "▼",
        category: "numpad",
    },
    Kp3: {
        label: "3",
        symbol: "下翻页",
        category: "numpad",
    },
    Kp4: {
        label: "4",
        symbol: "◀",
        category: "numpad",
    },
    Kp5: {
        label: "5",
        symbol: " ",
        category: "numpad",
    },
    Kp6: {
        label: "6",
        symbol: "▶",
        category: "numpad",
    },
    Kp7: {
        label: "7",
        symbol: "主页",
        category: "numpad",
    },
    Kp8: {
        label: "8",
        symbol: "▲",
        category: "numpad",
    },
    Kp9: {
        label: "9",
        symbol: "上翻页",
        category: "numpad",
    },
    // ───────────── Media ─────────────
    VolumeUp: {
        label: "音量加",
        shortLabel: "音量+",
        icon: Volume2Icon,
    },
    VolumeDown: {
        label: "音量减",
        shortLabel: "音量-",
        icon: Volume2Icon,
    },
    VolumeMute: {
        label: "静音",
        icon: VolumeXIcon,
    },

    // ───────────── Mouse Events ─────────────
    Left: {
        label: "左键单击",
        shortLabel: "左键",
        icon: MouseLeftClickIcon,
        category: "mouse",
    },
    Middle: {
        label: "中键单击",
        shortLabel: "中键",
        icon: MouseMiddleClickIcon,
        category: "mouse",
    },
    Right: {
        label: "右键单击",
        shortLabel: "右键",
        icon: MouseRightClickIcon,
        category: "mouse",
    },
    Drag: {
        label: "拖拽",
        icon: MouseRightDragIcon,
        category: "mouse",
    },
    ScrollUp: {
        label: "向上滚动",
        shortLabel: "滚动",
        icon: MouseScrollUpIcon,
        category: "mouse",
    },
    ScrollDown: {
        label: "向下滚动",
        shortLabel: "滚动",
        icon: MouseScrollDownIcon,
        category: "mouse",
    },
};

// ───────────── Apply Mappings for Modifiers ─────────────

// Control
['ControlLeft', 'ControlRight'].forEach((key) => {
    keymaps[key] = {
        label: "Ctrl",
        shortLabel: "Ctrl",
        glyph: "⌃",
        icon: ChevronUpIcon,
        category: "modifier",
    };
});

// Meta
['MetaLeft', 'MetaRight'].forEach((key) => {
    keymaps[key] = switchPlatform({
        windows: {
            label: "Win",
            glyph: "\u229E",
            icon: Grid2X2Icon,
            category: "modifier",
        },
        macos: {
            label: "Command",
            shortLabel: "Cmd",
            glyph: "⌘",
            icon: CommandIcon,
            category: "modifier",
        },
        linux: {
            label: "Meta",
            glyph: "✦",
            icon: SparkleIcon,
            category: "modifier",
        },
    });
});

// Alt
['Alt'].forEach((key) => {
    keymaps[key] = {
        label: switchPlatform({
            windows: "Alt",
            macos: "Option",
        }),
        shortLabel: switchPlatform({
            windows: "Alt",
            macos: "Opt",
        }),
        glyph: "⌥",
        icon: OptionIcon,
        category: "modifier",
    };
});

// Shift
['ShiftLeft', 'ShiftRight'].forEach((key) => {
    keymaps[key] = {
        label: "Shift",
        glyph: "⇧",
        icon: ArrowBigUpIcon,
        category: "modifier",
    };
});