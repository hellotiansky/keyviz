import { ColorInput } from "@/components/ui/color-picker";
import { Item, ItemActions, ItemContent, ItemDescription, ItemGrid, ItemTitle } from "@/components/ui/item";
import { NumberInput } from "@/components/ui/number-input";
import { Switch } from "@/components/ui/switch";
import { useKeyEvent } from "@/stores/key_event";
import { useKeyStyle } from '@/stores/key_style';
import { ArrowExpand02Icon, Cursor01Icon, CursorCircleSelection01Icon, CursorEdit01Icon, CursorMagicSelection03FreeIcons, Drag03Icon, Link02Icon, MouseLeftClick05Icon, PaintBoardIcon, Unlink02Icon } from "@hugeicons/core-free-icons";
import { HugeiconsIcon } from "@hugeicons/react";
import { NumberScrubber } from "../ui/number-input-scrub";
import { useState } from "react";
import { Toggle } from "../ui/toggle";


export const MouseSettings = () => {
    const mouse = useKeyStyle(state => state.mouse);
    const setMouseStyle = useKeyStyle(state => state.setMouse);

    const dragThreshold = useKeyEvent(state => state.dragThreshold);
    const setDragThreshold = useKeyEvent(state => state.setDragThreshold);

    const [offsetLinked, setOffsetLinked] = useState(true);

    return <div className="flex flex-col gap-y-4 p-6">
        <h1 className="text-xl font-semibold">鼠标</h1>

        <h2 className="text-sm text-muted-foreground font-medium">光标高亮</h2>
        <Item variant="muted">
            <ItemContent>
                <ItemTitle>
                    <HugeiconsIcon icon={CursorMagicSelection03FreeIcons} size="1em" /> 显示点击
                </ItemTitle>
                <ItemDescription>
                    按下鼠标时显示圆环动画
                </ItemDescription>
            </ItemContent>
            <ItemActions>
                <Switch
                    checked={mouse.showClicks}
                    onCheckedChange={(showClicks) => setMouseStyle({ showClicks })}
                />
            </ItemActions>
        </Item>

        <ItemGrid>
            <Item variant="muted">
                <ItemContent>
                    <ItemTitle>
                        <HugeiconsIcon icon={CursorCircleSelection01Icon} size="1em" /> 大小
                    </ItemTitle>
                </ItemContent>
                <ItemActions>
                    <NumberInput
                        step={10}
                        className="w-32 h-8"
                        value={mouse.size}
                        onChange={(size) => setMouseStyle({ size })}
                    />
                </ItemActions>
            </Item>

            <Item variant="muted">
                <ItemContent>
                    <ItemTitle>
                        <HugeiconsIcon icon={PaintBoardIcon} size="1em" /> 颜色
                    </ItemTitle>
                </ItemContent>
                <ItemActions>
                    <ColorInput
                        className="w-32"
                        value={mouse.color}
                        onChange={(color) => setMouseStyle({ color })}
                        disabled={!mouse.showClicks}
                    />
                </ItemActions>
            </Item>
        </ItemGrid>

        <Item variant="muted">
            <ItemContent>
                <ItemTitle>
                    <HugeiconsIcon icon={Cursor01Icon} size="1em" /> 始终高亮
                </ItemTitle>
                <ItemDescription>
                    始终在光标周围显示圆环
                </ItemDescription>
            </ItemContent>
            <ItemActions>
                <Switch
                    checked={mouse.keepHighlight}
                    onCheckedChange={(keepHighlight) => setMouseStyle({ keepHighlight })}
                    disabled={!mouse.showClicks}
                />
            </ItemActions>
        </Item>

        <h2 className="text-sm text-muted-foreground font-medium mt-2">按键指示器</h2>
        <Item variant="muted">
            <ItemContent>
                <ItemTitle>
                    <HugeiconsIcon icon={MouseLeftClick05Icon} size="1em" /> 显示指示器
                </ItemTitle>
                <ItemDescription>
                    在光标旁边显示按键和滚动图标
                </ItemDescription>
            </ItemContent>
            <ItemActions>
                <Switch
                    checked={mouse.showIndicator}
                    onCheckedChange={(showIndicator) => setMouseStyle({ showIndicator })}
                />
            </ItemActions>
        </Item>

        <Item variant="muted">
            <ItemContent>
                <ItemTitle>
                    <HugeiconsIcon icon={Cursor01Icon} size="1em" /> 保持显示
                </ItemTitle>
                <ItemDescription>
                    始终在光标旁边显示图标
                </ItemDescription>
            </ItemContent>
            <ItemActions>
                <Switch
                    checked={mouse.keepIndicator}
                    onCheckedChange={(keepIndicator) => setMouseStyle({ keepIndicator })}
                    disabled={!mouse.showIndicator}
                />
            </ItemActions>
        </Item>

        <Item variant="muted">
            <ItemContent>
                <ItemTitle>
                    <HugeiconsIcon icon={CursorEdit01Icon} size="1em" /> 大小
                </ItemTitle>
            </ItemContent>
            <ItemActions>
                <NumberInput
                    className="w-32 h-8"
                    value={mouse.indicatorSize}
                    onChange={(indicatorSize) => setMouseStyle({ indicatorSize })}
                />
            </ItemActions>
        </Item>

        <Item variant="muted">
            <ItemContent>
                <ItemTitle>
                    <HugeiconsIcon icon={ArrowExpand02Icon} size="1em" /> 偏移
                </ItemTitle>
                <ItemDescription>
                    指示器与光标之间的距离
                </ItemDescription>
            </ItemContent>
            <ItemActions>
                <NumberScrubber
                    value={mouse.indicatorOffsetX}
                    onChange={offsetLinked ? (indicatorOffsetX => { setMouseStyle({ indicatorOffsetX }); setMouseStyle({ indicatorOffsetY: indicatorOffsetX }); }) : (indicatorOffsetX => setMouseStyle({ indicatorOffsetX }))}
                    step={1}
                    icon={<span className="ml-0.5 text-xs font-medium">X</span>}
                    className="w-18"
                />
                <Toggle
                    variant="default"
                    pressed={offsetLinked}
                    onPressedChange={(pressed) => {
                        setOffsetLinked(pressed);
                        if (pressed) {
                            setMouseStyle({ indicatorOffsetY: mouse.indicatorOffsetX });
                        }
                    }}
                    aria-label="偏移联动"
                >
                    <HugeiconsIcon icon={offsetLinked ? Link02Icon : Unlink02Icon} size="1em" />
                </Toggle>
                <NumberScrubber
                    value={mouse.indicatorOffsetY}
                    onChange={(indicatorOffsetY) => setMouseStyle({ indicatorOffsetY })}
                    step={1}
                    icon={<span className="ml-0.5 text-xs font-medium">Y</span>}
                    className="w-18"
                    disabled={offsetLinked}
                />
            </ItemActions>
        </Item>

        <h2 className="text-sm text-muted-foreground font-medium mt-2">事件</h2>
        <Item variant="muted">
            <ItemContent>
                <ItemTitle>
                    <HugeiconsIcon icon={Drag03Icon} size="1em" /> 拖拽阈值
                </ItemTitle>
                <ItemDescription>
                    触发拖拽事件所需的最小像素距离
                </ItemDescription>
            </ItemContent>
            <ItemActions>
                <NumberInput
                    className="w-32 h-8"
                    value={dragThreshold}
                    onChange={setDragThreshold}
                />
            </ItemActions>
        </Item>
    </div>;
}