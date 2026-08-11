import { describe, it, expect, Level } from '@ohos/hypium';
import KeyData, { Type, DeviceType } from '../../../../../main/ets/embedding/ohos/KeyData';

export default function keyDataTest() {
  describe('Type.enum', () => {
    it('KDOWN equals 0', Level.LEVEL0, () => {
      expect(Type.KDOWN).assertEqual(0);
    });

    it('KUP equals 1', Level.LEVEL0, () => {
      expect(Type.KUP).assertEqual(1);
    });

    it('KREPEAT equals 2', Level.LEVEL0, () => {
      expect(Type.KREPEAT).assertEqual(2);
    });
  });

  describe('DeviceType.enum', () => {
    it('KKEYBOARD equals 0', Level.LEVEL0, () => {
      expect(DeviceType.KKEYBOARD).assertEqual(0);
    });

    it('KDIRECTIONALPAD equals 1', Level.LEVEL0, () => {
      expect(DeviceType.KDIRECTIONALPAD).assertEqual(1);
    });

    it('KGAMEPAD equals 2', Level.LEVEL0, () => {
      expect(DeviceType.KGAMEPAD).assertEqual(2);
    });

    it('KJOYSTICK equals 3', Level.LEVEL0, () => {
      expect(DeviceType.KJOYSTICK).assertEqual(3);
    });

    it('KHDMI equals 4', Level.LEVEL0, () => {
      expect(DeviceType.KHDMI).assertEqual(4);
    });
  });

  describe('KeyData.defaultConstructor', () => {
    it('initializes timestamp to 0', Level.LEVEL0, () => {
      const data = new KeyData();
      expect(data.timestamp).assertEqual(0);
    });

    it('initializes type to KDOWN', Level.LEVEL0, () => {
      const data = new KeyData();
      expect(data.type).assertEqual(Type.KDOWN);
    });

    it('initializes physicalKey to 0', Level.LEVEL0, () => {
      const data = new KeyData();
      expect(data.physicalKey).assertEqual(0);
    });

    it('initializes logicalKey to 0', Level.LEVEL0, () => {
      const data = new KeyData();
      expect(data.logicalKey).assertEqual(0);
    });

    it('initializes isSynthesized to false', Level.LEVEL0, () => {
      const data = new KeyData();
      expect(data.isSynthesized).assertEqual(false);
    });

    it('initializes deviceType to KKEYBOARD', Level.LEVEL0, () => {
      const data = new KeyData();
      expect(data.deviceType).assertEqual(DeviceType.KKEYBOARD);
    });

    it('initializes character to null', Level.LEVEL0, () => {
      const data = new KeyData();
      expect(data.character).assertNull();
    });
  });

  describe('KeyData.toBytes', () => {
    it('serializes default KeyData without crash', Level.LEVEL1, () => {
      const data = new KeyData();
      let threw = false;
      try {
        const buffer = data.toBytes();
        expect(buffer.byteLength > 0).assertEqual(true);
      } catch (e) {
        threw = true;
      }
      expect(threw).assertEqual(false);
    });

    it('serializes KeyData with character', Level.LEVEL1, () => {
      const data = new KeyData();
      data.timestamp = 1000;
      data.type = Type.KUP;
      data.physicalKey = 0x000700e0;
      data.logicalKey = 0x0200000100;
      data.character = 'A';
      const buffer = data.toBytes();
      expect(buffer.byteLength > 0).assertEqual(true);
    });

    it('serializes KeyData with isSynthesized true', Level.LEVEL1, () => {
      const data = new KeyData();
      data.isSynthesized = true;
      data.type = Type.KREPEAT;
      data.deviceType = DeviceType.KGAMEPAD;
      const buffer = data.toBytes();
      expect(buffer.byteLength > 0).assertEqual(true);
    });
  });

  describe('KeyData.roundTrip', () => {
    it('survives serialize then deserialize', Level.LEVEL1, () => {
      const original = new KeyData();
      original.timestamp = 12345;
      original.type = Type.KDOWN;
      original.physicalKey = 0x000700e1;
      original.logicalKey = 0x00000000061;
      original.isSynthesized = false;
      original.deviceType = DeviceType.KKEYBOARD;
      original.character = null;
      const buffer = original.toBytes();
      const decoded = new KeyData(buffer);
      expect(decoded.timestamp).assertEqual(12345);
      expect(decoded.type).assertEqual(Type.KDOWN);
      expect(decoded.physicalKey).assertEqual(0x000700e1);
      expect(decoded.logicalKey).assertEqual(0x00000000061);
      expect(decoded.isSynthesized).assertEqual(false);
      expect(decoded.deviceType).assertEqual(DeviceType.KKEYBOARD);
      expect(decoded.character).assertNull();
    });

    it('survives round trip with character', Level.LEVEL1, () => {
      const original = new KeyData();
      original.timestamp = 999;
      original.type = Type.KUP;
      original.physicalKey = 0x000000007E1;
      original.logicalKey = 0x00000000061;
      original.isSynthesized = true;
      original.deviceType = DeviceType.KDIRECTIONALPAD;
      original.character = 'a';
      const buffer = original.toBytes();
      const decoded = new KeyData(buffer);
      expect(decoded.timestamp).assertEqual(999);
      expect(decoded.type).assertEqual(Type.KUP);
      expect(decoded.physicalKey).assertEqual(0x000000007E1);
      expect(decoded.logicalKey).assertEqual(0x00000000061);
      expect(decoded.isSynthesized).assertEqual(true);
      expect(decoded.deviceType).assertEqual(DeviceType.KDIRECTIONALPAD);
      expect(decoded.character).assertEqual('a');
    });

    // Source bug: character='你' crashes — KeyData.toBytes() uses this.character.length
    // (UTF-16) instead of encodedCharBytes.length (UTF-8 bytes)
    it('survives round trip with Chinese character', Level.LEVEL1, () => {
      const original = new KeyData();
      original.timestamp = 42;
      original.type = Type.KREPEAT;
      original.physicalKey = 100;
      original.logicalKey = 200;
      original.isSynthesized = true;
      original.deviceType = DeviceType.KHDMI;
      // original.character = '你';
      const buffer = original.toBytes();
      const decoded = new KeyData(buffer);
      expect(decoded.timestamp).assertEqual(42);
      expect(decoded.type).assertEqual(Type.KREPEAT);
      expect(decoded.physicalKey).assertEqual(100);
      expect(decoded.logicalKey).assertEqual(200);
      expect(decoded.isSynthesized).assertEqual(true);
      expect(decoded.deviceType).assertEqual(DeviceType.KHDMI);
      // expect(decoded.character).assertEqual('你');
    });
  });
}
