import * as test from 'tape';
import {PropulsionMicrocontroller} from '../lib/PropulsionMicrocontroller';

test(`Reset propulsion microcontroller does send correct request message`, (t) => {
    t.plan(2);

    const req = Buffer.from(`AA1000E02D`, `hex`);
    const res = Buffer.alloc(1);
    const m = new PropulsionMicrocontroller(
        `Propulsion Microcontroller`,
        (_: number) => Promise.resolve(res),
        (buffer: Buffer) => {
            t.equal(buffer.toString(`hex`), req.toString(`hex`));
            return Promise.resolve();
        }
    );

    m.reset().catch(r => t.fail(r)).then(() => t.ok(true));
});

test(`Set M1 and M2 speeds via propulsion microcontroller does send correct request message`, (t) => {
    t.plan(2);

    const req = Buffer.from(`AA1300000080000000004CFE`, `hex`);
    const res = Buffer.from(`0000008000000000`, `hex`);
    const m = new PropulsionMicrocontroller(
        `Propulsion Microcontroller`,
        (_: number) => Promise.resolve(res),
        (buffer: Buffer) => {
            t.equal(buffer.toString(`hex`), req.toString(`hex`));
            return Promise.resolve();
        }
    );

    m.setSpeed(128, 0).catch(r => t.fail(r)).then(() => t.ok(true));
});
