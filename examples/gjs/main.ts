#!/usr/bin/env -S gjs -m

import Gxr from 'gi://Gxr';
import GLib from 'gi://GLib';

const Context = Gxr.Context

const ctx = Context.new("Pose Test", 1);

let i = 0
while (i < 100) {
    let [ret, pose] = ctx.get_head_pose();
    if (ret) {
        pose.print()
    } else {
        console.log('no pose received')
    }

    GLib.usleep(50_000)
    i++
}
