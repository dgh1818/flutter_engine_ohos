import { describe, expect, it, Level } from '@ohos/hypium';

export default function smokeTest() {
  describe('smoke', () => {
    it('framework_alive', Level.LEVEL0, () => {
      expect(1 + 1).assertEqual(2);
    });

    it('string_eq', Level.LEVEL0, () => {
      expect('flutter').assertContain('flut');
    });
  });
}
