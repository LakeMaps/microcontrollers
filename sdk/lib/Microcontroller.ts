export class Microcontroller {
    constructor(readonly name: string, readonly read: (n: number) => Promise<Buffer>, readonly write: (b: Buffer) => Promise<number>) {
        // ???
    }
}
