import * as test from 'tape';
import {Message} from '../lib/Message';

test(`Message#payload does return slice with correct size and data`, (t) => {
    t.plan(2);

    const message = new Message(0x1, Buffer.alloc(42, `x`));
    t.equal(42, message.payload().length);
    t.ok(message.payload().equals(Buffer.alloc(42, `x`)));
});

test(`Message#payload does return slice with correct size and data for message without payload`, (t) => {
    t.plan(1);

    const message = new Message(0x1, Buffer.alloc(0));
    t.equal(0, message.payload().length);
});
