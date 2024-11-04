#!/usr/bin/env -S gjs -m

import Gxr from 'gi://Gxr';
import GLib from 'gi://GLib';

const Context = Gxr.Context

const ctx = Context.new("Pose Test", 1);

let i = 0
while (i < 500) {
    // Calling begin/end frame allows to set `predicted_display_time` internally
    // Without that `get_head_pose()` returns delayed data
    ctx.begin_frame();
    ctx.end_frame();

    let [ret, pose] = ctx.get_head_pose();
    if (ret) {
        pose.print()
    } else {
        console.log('no pose received')
    }

    // Needed to follow correct call order with begin/end frame.
    // It also controls loop rate
    ctx.wait_frame();
    i++
}
