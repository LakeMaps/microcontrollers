import * as test from 'tape';
import {WirelessCommunicationsMicrocontroller} from '../lib/WirelessCommunicationsMicrocontroller';

test(`Reset wireless controller does send correct request message`, (t) => {
    t.plan(2);

    const req = Buffer.from(`AA00007A5D`, `hex`);
    const res = Buffer.from(`AA00007A5D`, `hex`);
    const m = new WirelessCommunicationsMicrocontroller(
        `Propulsion Microcontroller`,
        (_: number) => Promise.resolve(res),
        (buffer: Buffer) => {
            t.equal(buffer.toString(`hex`), req.toString(`hex`));
            return Promise.resolve(buffer.length);
        }
    );

    m.reset().catch(r => t.fail(r)).then(() => t.ok(true));
});

test(`Send wireless controller does send correct request message`, (t) => {
    t.plan(2);

    const req = Buffer.from(
        `AA0400000000000000000000000000000000000000333333333333333333333333333333333333333333333333333333333333333333333333333333333333BFC3`, `hex`
    );
    const res = Buffer.from(`AA0401A6B8`, `hex`);
    const m = new WirelessCommunicationsMicrocontroller(
        `Propulsion Microcontroller`,
        (_: number) => Promise.resolve(res),
        (buffer: Buffer) => {
            t.equal(buffer.toString(`hex`), req.toString(`hex`));
            return Promise.resolve(buffer.length);
        }
    );

    m.send(Buffer.alloc(42, `3`)).catch(r => t.fail(r)).then(() => t.ok(true));
});
