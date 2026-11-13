import { invoke } from '@tauri-apps/api/core';

import { ShortcutRecorder } from '@/components/shortcut-recorder';
import { Button } from '@/components/ui/button';
import {
    Drawer,
    DrawerContent,
    DrawerDescription,
    DrawerHeader,
    DrawerTitle,
    DrawerTrigger,
} from "@/components/ui/drawer";
import { Item, ItemActions, ItemContent, ItemDescription, ItemHeader, ItemTitle } from "@/components/ui/item";
import { NumberInput } from '@/components/ui/number-input';
import { Switch } from "@/components/ui/switch";
import { ToggleGroup, ToggleGroupItem } from '@/components/ui/toggle-group';
import { cn } from "@/lib/utils";
import { KeyEventState, useKeyEvent } from "@/stores/key_event";
import { KeyStyleState, useKeyStyle } from "@/stores/key_style";
import { ArrowHorizontalIcon, ArrowVerticalIcon, FilterHorizontalIcon, FilterIcon, LayerIcon, ToggleOnIcon } from "@hugeicons/core-free-icons";
import { HugeiconsIcon } from "@hugeicons/react";
import { CustomFilter } from '../custom-filter';


export const GeneralSettings = () => {
    const {
        filter, setFilter,
        allowedKeys,
        showEventHistory, setShowEventHistory,
        maxHistory, setMaxHistory,
        toggleShortcut, setToggleShortcut
    } = useKeyEvent();

    const direction = useKeyStyle(state => state.appearance.flexDirection);
    const setAppearance = useKeyStyle(state => state.setAppearance);

    return <div className="flex flex-col gap-y-4 p-6">
        <h1 className="text-xl font-semibold">常规</h1>

        <Item variant="muted">
            <ItemContent>
                <ItemTitle>
                    <HugeiconsIcon icon={FilterIcon} size="1em" /> 过滤器
                </ItemTitle>
                <ItemDescription>
                    {filter === 'none' && '未启用过滤，将显示所有按键。'}
                    {filter === 'modifiers' && '仅显示修饰键。'}
                    {filter === 'custom' && `已启用自定义过滤，允许 ${allowedKeys.length} 个按键。`}
                </ItemDescription>
            </ItemContent>
            <ItemActions>
                {
                    filter === 'custom' &&
                    <Drawer>
                        <DrawerTrigger asChild>
                            <Button variant="outline" size="icon-sm">
                                <HugeiconsIcon icon={FilterHorizontalIcon} />
                            </Button>
                        </DrawerTrigger>
                        <DrawerContent>
                            <DrawerContent>
                                <DrawerHeader>
                                    <DrawerTitle>自定义过滤器</DrawerTitle>
                                    <DrawerDescription>选择要显示的按键。按住 Ctrl 可批量切换同组按键。</DrawerDescription>
                                </DrawerHeader>
                                <CustomFilter />
                            </DrawerContent>
                        </DrawerContent>
                    </Drawer>
                }
                <ToggleGroup
                    size="sm"
                    type="single"
                    variant="outline"
                    value={filter}
                    onValueChange={(value) => setFilter(value as KeyEventState["filter"])}
                >
                    <ToggleGroupItem value="none" aria-label="不过滤">关闭</ToggleGroupItem>
                    <ToggleGroupItem value="modifiers" aria-label="仅修饰键">热键</ToggleGroupItem>
                    <ToggleGroupItem value="custom" aria-label="自定义过滤">自定义</ToggleGroupItem>
                </ToggleGroup>
            </ItemActions>
        </Item>

        <Item variant="muted">
            <ItemContent>
                <ItemTitle>
                    <HugeiconsIcon icon={LayerIcon} size="1em" /> 历史记录
                </ItemTitle>
                <ItemDescription>
                    在画面中保留之前按下的按键
                </ItemDescription>
            </ItemContent>
            <ItemActions>
                <Switch checked={showEventHistory} onCheckedChange={setShowEventHistory} />
            </ItemActions>
        </Item>

        <div className={cn("flex flex-col gap-4 md:flex-row", showEventHistory ? "" : "pointer-events-none opacity-50", "transition-opacity")}>
            <Item variant="muted" className="flex-7">
                <ItemContent>
                    <ItemTitle>排列方向</ItemTitle>
                </ItemContent>
                <ItemActions>
                    <ToggleGroup
                        size="sm"
                        type="single"
                        variant="outline"
                        value={direction}
                        onValueChange={(value) => setAppearance({ flexDirection: value as KeyStyleState["appearance"]["flexDirection"] })}
                    >
                        <ToggleGroupItem value="row" aria-label="水平">
                            <HugeiconsIcon icon={ArrowHorizontalIcon} strokeWidth={2} size={10} /> 水平
                        </ToggleGroupItem>
                        <ToggleGroupItem value="column" aria-label="垂直">
                            <HugeiconsIcon icon={ArrowVerticalIcon} strokeWidth={2} /> 垂直
                        </ToggleGroupItem>
                    </ToggleGroup>
                </ItemActions>
            </Item>
            <Item variant="muted" className="flex-5">
                <ItemContent>
                    <ItemTitle>最大数量</ItemTitle>
                </ItemContent>
                <ItemActions className="max-w-20">
                    <NumberInput className="h-8" value={maxHistory} onChange={setMaxHistory} minValue={2} maxValue={12} />
                </ItemActions>
            </Item>
        </div>

        <Item variant="muted">
            <ItemHeader className="flex-col items-start">
                <ItemTitle>
                    <HugeiconsIcon icon={ToggleOnIcon} size="1em" /> 开关快捷键
                </ItemTitle>
                <ItemDescription>
                    用于显示/隐藏按键可视化的全局快捷键，点击方框即可设置
                </ItemDescription>
            </ItemHeader>
            <ItemContent>
                <ShortcutRecorder value={toggleShortcut} onChange={shortcut => {
                    setToggleShortcut(shortcut);
                    invoke('set_toggle_shortcut', { shortcut });
                }} />
            </ItemContent>
        </Item>
    </div>;
}