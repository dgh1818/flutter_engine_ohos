import { describe, it, expect, Level } from '@ohos/hypium';
import KeyboardMap, { KeyPair, ModifierGoal, ModifierKeyMetaInfo } from '../../../../../main/ets/embedding/ohos/KeyboardMap';

export default function keyboardMapTest() {
  describe('KeyPair.constructor', () => {
    it('assigns fields', Level.LEVEL0, () => { const p = new KeyPair(100, 200); expect(p.physicalKey).assertEqual(100); expect(p.logicalKey).assertEqual(200); });
    it('handles zero', Level.LEVEL0, () => { const p = new KeyPair(0, 0); expect(p.physicalKey).assertEqual(0); expect(p.logicalKey).assertEqual(0); });
  });
  describe('ModifierGoal.constructor', () => {
    it('assigns name and keys', Level.LEVEL0, () => { const ks: KeyPair[] = [new KeyPair(1,2)]; const g = new ModifierGoal('Ctrl', ks); expect(g.name).assertEqual('Ctrl'); expect(g.keys.length).assertEqual(1); });
  });
  describe('KeyboardMap.toLogicalKey', () => {
    it('endCall_1', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x000000007D0)).assertEqual(0x00000000030);
    });
    it('endCall_2', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x000000007D1)).assertEqual(0x00000000031);
    });
    it('endCall_3', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x000000007D2)).assertEqual(0x00000000032);
    });
    it('endCall_4', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x000000007D3)).assertEqual(0x00000000033);
    });
    it('endCall_5', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x000000007D4)).assertEqual(0x00000000034);
    });
    it('endCall_6', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x000000007D5)).assertEqual(0x00000000035);
    });
    it('endCall_7', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x000000007D6)).assertEqual(0x00000000036);
    });
    it('endCall_8', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x000000007D7)).assertEqual(0x00000000037);
    });
    it('endCall_9', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x000000007D8)).assertEqual(0x00000000038);
    });
    it('endCall_10', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x000000007D9)).assertEqual(0x00000000039);
    });
    it('endCall_11', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x000000007DA)).assertEqual(0x0000000002A);
    });
    it('endCall_12', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x000000007DB)).assertEqual(0x00000000023);
    });
    it('endCall_13', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x000000007DC)).assertEqual(0x00100000304);
    });
    it('endCall_14', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x000000007DD)).assertEqual(0x00100000301);
    });
    it('endCall_15', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x000000007DE)).assertEqual(0x00100000302);
    });
    it('endCall_16', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x000000007DF)).assertEqual(0x00100000303);
    });
    it('endCall_17', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x000000007E1)).assertEqual(0x00000000061);
    });
    it('endCall_18', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x000000007E2)).assertEqual(0x00000000062);
    });
    it('endCall_19', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x000000007E3)).assertEqual(0x00000000063);
    });
    it('endCall_20', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x000000007E4)).assertEqual(0x00000000064);
    });
    it('endCall_21', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x000000007E5)).assertEqual(0x00000000065);
    });
    it('endCall_22', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x000000007E6)).assertEqual(0x00000000066);
    });
    it('endCall_23', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x000000007E7)).assertEqual(0x00000000067);
    });
    it('endCall_24', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x000000007E8)).assertEqual(0x00000000068);
    });
    it('endCall_25', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x000000007E9)).assertEqual(0x00000000069);
    });
    it('endCall_26', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x000000007EA)).assertEqual(0x0000000006A);
    });
    it('endCall_27', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x000000007EB)).assertEqual(0x0000000006B);
    });
    it('endCall_28', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x000000007EC)).assertEqual(0x0000000006C);
    });
    it('endCall_29', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x000000007ED)).assertEqual(0x0000000006D);
    });
    it('endCall_30', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x000000007EE)).assertEqual(0x0000000006E);
    });
    it('endCall_31', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x000000007EF)).assertEqual(0x0000000006F);
    });
    it('endCall_32', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x000000007F0)).assertEqual(0x00000000070);
    });
    it('endCall_33', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x000000007F1)).assertEqual(0x00000000071);
    });
    it('endCall_34', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x000000007F2)).assertEqual(0x00000000072);
    });
    it('endCall_35', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x000000007F3)).assertEqual(0x00000000073);
    });
    it('endCall_36', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x000000007F4)).assertEqual(0x00000000074);
    });
    it('endCall_37', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x000000007F5)).assertEqual(0x00000000075);
    });
    it('endCall_38', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x000000007F6)).assertEqual(0x00000000076);
    });
    it('endCall_39', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x000000007F7)).assertEqual(0x00000000077);
    });
    it('endCall_40', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x000000007F8)).assertEqual(0x00000000078);
    });
    it('endCall_41', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x000000007F9)).assertEqual(0x00000000079);
    });
    it('endCall_42', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x000000007FA)).assertEqual(0x0000000007A);
    });
    it('endCall_43', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x000000007FB)).assertEqual(0x0000000002C);
    });
    it('endCall_44', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x000000007FC)).assertEqual(0x0000000002E);
    });
    it('endCall_45', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x000000007FD)).assertEqual(0x00200000104);
    });
    it('endCall_46', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x000000007FE)).assertEqual(0x00200000105);
    });
    it('endCall_47', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x000000007FF)).assertEqual(0x00200000102);
    });
    it('endCall_48', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000800)).assertEqual(0x00200000103);
    });
    it('endCall_49', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000801)).assertEqual(0x00100000009);
    });
    it('endCall_50', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000802)).assertEqual(0x00000000020);
    });
    it('endCall_51', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000804)).assertEqual(0x00100000B09);
    });
    it('endCall_52', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000805)).assertEqual(0x00100000B03);
    });
    it('endCall_53', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000806)).assertEqual(0x0010000000D);
    });
    it('endCall_54', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000807)).assertEqual(0x00100000008);
    });
    it('endCall_55', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000808)).assertEqual(0x00000000060);
    });
    it('endCall_56', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000809)).assertEqual(0x0000000002D);
    });
    it('endCall_57', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x0000000080A)).assertEqual(0x0000000003D);
    });
    it('endCall_58', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x0000000080B)).assertEqual(0x0000000005B);
    });
    it('endCall_59', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x0000000080C)).assertEqual(0x0000000005D);
    });
    it('endCall_60', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x0000000080D)).assertEqual(0x0000000005C);
    });
    it('endCall_61', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x0000000080E)).assertEqual(0x0000000003B);
    });
    it('endCall_62', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x0000000080F)).assertEqual(0x00000000022);
    });
    it('endCall_63', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000810)).assertEqual(0x0000000002F);
    });
    it('endCall_64', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000813)).assertEqual(0x00100000505);
    });
    it('endCall_65', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x000000009A2)).assertEqual(0x00100000704);
    });
    it('endCall_66', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000814)).assertEqual(0x00100000308);
    });
    it('endCall_67', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000815)).assertEqual(0x00100000307);
    });
    it('endCall_68', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000816)).assertEqual(0x0010000001B);
    });
    it('endCall_69', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000817)).assertEqual(0x0010000007F);
    });
    it('endCall_70', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000818)).assertEqual(0x00200000100);
    });
    it('endCall_71', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000819)).assertEqual(0x00200000101);
    });
    it('endCall_72', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x0000000081A)).assertEqual(0x00100000104);
    });
    it('endCall_73', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x0000000081B)).assertEqual(0x0010000010C);
    });
    it('endCall_74', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x0000000081C)).assertEqual(0x00200000106);
    });
    it('endCall_75', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x0000000081D)).assertEqual(0x00200000107);
    });
    it('endCall_76', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x0000000081E)).assertEqual(0x00100000106);
    });
    it('endCall_77', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x0000000081F)).assertEqual(0x00100000608);
    });
    it('endCall_78', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000820)).assertEqual(0x00100000509);
    });
    it('endCall_79', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000821)).assertEqual(0x00100000306);
    });
    it('endCall_80', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000822)).assertEqual(0x00100000305);
    });
    it('endCall_81', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000823)).assertEqual(0x00100000407);
    });
    it('endCall_82', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000824)).assertEqual(0x00100000C03);
    });
    it('endCall_83', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000825)).assertEqual(0x00100000D2F);
    });
    it('endCall_84', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000A53)).assertEqual(0x0010000050A);
    });
    it('endCall_85', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000826)).assertEqual(0x00100000D2E);
    });
    it('endCall_86', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000827)).assertEqual(0x00100000D5B);
    });
    it('endCall_87', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000828)).assertEqual(0x00100000604);
    });
    it('endCall_88', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000829)).assertEqual(0x00100000D30);
    });
    it('endCall_89', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x0000000082A)).assertEqual(0x00100000801);
    });
    it('endCall_90', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x0000000082B)).assertEqual(0x00100000802);
    });
    it('endCall_91', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x0000000082C)).assertEqual(0x00100000803);
    });
    it('endCall_92', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x0000000082D)).assertEqual(0x00100000804);
    });
    it('endCall_93', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x0000000082E)).assertEqual(0x00100000805);
    });
    it('endCall_94', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x0000000082F)).assertEqual(0x00100000806);
    });
    it('endCall_95', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000830)).assertEqual(0x00100000807);
    });
    it('endCall_96', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000831)).assertEqual(0x00100000808);
    });
    it('endCall_97', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000832)).assertEqual(0x00100000809);
    });
    it('endCall_98', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000833)).assertEqual(0x0010000080A);
    });
    it('endCall_99', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000834)).assertEqual(0x0010000080B);
    });
    it('endCall_100', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000835)).assertEqual(0x0010000080C);
    });
    it('endCall_101', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000836)).assertEqual(0x0010000010A);
    });
    it('endCall_102', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000837)).assertEqual(0x00200000230);
    });
    it('endCall_103', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000838)).assertEqual(0x00200000231);
    });
    it('endCall_104', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000839)).assertEqual(0x00200000232);
    });
    it('endCall_105', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x0000000083A)).assertEqual(0x00200000233);
    });
    it('endCall_106', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x0000000083B)).assertEqual(0x00200000234);
    });
    it('endCall_107', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x0000000083C)).assertEqual(0x00200000235);
    });
    it('endCall_108', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x0000000083D)).assertEqual(0x00200000236);
    });
    it('endCall_109', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x0000000083E)).assertEqual(0x00200000237);
    });
    it('endCall_110', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x0000000083F)).assertEqual(0x00200000238);
    });
    it('endCall_111', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000840)).assertEqual(0x00200000239);
    });
    it('endCall_112', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000841)).assertEqual(0x0020000022F);
    });
    it('endCall_113', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000842)).assertEqual(0x0020000022A);
    });
    it('endCall_114', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000843)).assertEqual(0x0020000022D);
    });
    it('endCall_115', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000844)).assertEqual(0x0020000022B);
    });
    it('endCall_116', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000845)).assertEqual(0x0020000022E);
    });
    it('endCall_117', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000846)).assertEqual(0x0020000022C);
    });
    it('endCall_118', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000847)).assertEqual(0x0020000020D);
    });
    it('endCall_119', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000848)).assertEqual(0x0020000023D);
    });
    it('endCall_120', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000849)).assertEqual(0x00200000228);
    });
    it('endCall_121', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x0000000084A)).assertEqual(0x00200000229);
    });
    it('endCall_122', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000010)).assertEqual(0x00100000A10);
    });
    it('endCall_123', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000011)).assertEqual(0x00100000A0F);
    });
    it('endCall_124', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000012)).assertEqual(0x00100000606);
    });
    it('endCall_125', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000016)).assertEqual(0x00100000E09);
    });
    it('endCall_126', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000001)).assertEqual(0x00100000306);
    });
    it('endCall_127', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000002)).assertEqual(0x00100001005);
    });
    it('endCall_128', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000013)).assertEqual(0x00100000603);
    });
    it('endCall_129', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000028)).assertEqual(0x00100000602);
    });
    it('endCall_130', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000029)).assertEqual(0x00100000601);
    });
    it('endCall_131', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000005)).assertEqual(0x00100000401);
    });
    it('endCall_132', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x0000000000A)).assertEqual(0x00100000A05);
    });
    it('endCall_133', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x0000000000B)).assertEqual(0x00100000A07);
    });
    it('endCall_134', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x0000000000C)).assertEqual(0x00100000A08);
    });
    it('endCall_135', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x0000000000D)).assertEqual(0x00100000A09);
    });
    it('endCall_136', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x0000000000E)).assertEqual(0x00100000D31);
    });
    it('endCall_137', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x0000000000F)).assertEqual(0x00100000D2C);
    });
    it('endCall_138', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000A28)).assertEqual(0x00200000002);
    });
    it('endCall_139', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000A29)).assertEqual(0x0010000071D);
    });
    it('endCall_140', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000A2C)).assertEqual(0x0010000071A);
    });
    it('endCall_141', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000A2D)).assertEqual(0x00100000716);
    });
    it('endCall_142', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000A2E)).assertEqual(0x00100000705);
    });
    it('endCall_143', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000A2F)).assertEqual(0x00100000717);
    });
    it('endCall_144', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000A30)).assertEqual(0x0010000070D);
    });
    it('endCall_145', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000A37)).assertEqual(0x00200000022);
    });
    it('endCall_146', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000A39)).assertEqual(0x00100000502);
    });
    it('endCall_147', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000A3A)).assertEqual(0x0010000050B);
    });
    it('endCall_148', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000A3B)).assertEqual(0x0010000040A);
    });
    it('endCall_149', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000A3C)).assertEqual(0x00100000402);
    });
    it('endCall_150', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000A3D)).assertEqual(0x00100000A0B);
    });
    it('endCall_151', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000A3E)).assertEqual(0x00100000408);
    });
    it('endCall_152', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000A3F)).assertEqual(0x00100000507);
    });
    it('endCall_153', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000A40)).assertEqual(0x00100000404);
    });
    it('endCall_154', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000A41)).assertEqual(0x00100000508);
    });
    it('endCall_155', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000A44)).assertEqual(0x00100000C02);
    });
    it('endCall_156', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000A46)).assertEqual(0x00100000A05);
    });
    it('endCall_157', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000A48)).assertEqual(0x00100000A01);
    });
    it('endCall_158', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000003)).assertEqual(0x00100001002);
    });
    it('endCall_159', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000A4B)).assertEqual(0x00100000C05);
    });
    it('endCall_160', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000A4C)).assertEqual(0x00100000D15);
    });
    it('endCall_161', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000A51)).assertEqual(0x00100000409);
    });
    it('endCall_162', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000A52)).assertEqual(0x00100000A01);
    });
    it('endCall_163', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000A55)).assertEqual(0x00100000A0C);
    });
    it('endCall_164', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000A58)).assertEqual(0x00100000504);
    });
    it('endCall_165', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000A5F)).assertEqual(0x00100000A0D);
    });
    it('endCall_166', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000A68)).assertEqual(0x00100000D25);
    });
    it('endCall_167', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000A6B)).assertEqual(0x00100000D47);
    });
    it('endCall_168', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000A70)).assertEqual(0x00100000D49);
    });
    it('endCall_169', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000A7E)).assertEqual(0x00100000D0C);
    });
    it('endCall_170', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000A7F)).assertEqual(0x00100000D0D);
    });
    it('endCall_171', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000A80)).assertEqual(0x00100000D0E);
    });
    it('endCall_172', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000A81)).assertEqual(0x00100000D0F);
    });
    it('endCall_173', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000A82)).assertEqual(0x00100000D0B);
    });
    it('endCall_174', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000A83)).assertEqual(0x00100000D0A);
    });
    it('endCall_175', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000A8A)).assertEqual(0x0010000050D);
    });
    it('endCall_176', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000A8B)).assertEqual(0x0010000050E);
    });
    it('endCall_177', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000A98)).assertEqual(0x00100000A0E);
    });
    it('endCall_178', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000AF2)).assertEqual(0x0010000060B);
    });
    it('endCall_179', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000AFD)).assertEqual(0x00100000604);
    });
    it('endCall_180', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000B00)).assertEqual(0x0010000080D);
    });
    it('endCall_181', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000B01)).assertEqual(0x0010000080E);
    });
    it('endCall_182', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000B02)).assertEqual(0x0010000080F);
    });
    it('endCall_183', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000B03)).assertEqual(0x00100000810);
    });
    it('endCall_184', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000B04)).assertEqual(0x00100000811);
    });
    it('endCall_185', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000B05)).assertEqual(0x00100000812);
    });
    it('endCall_186', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000B06)).assertEqual(0x00100000813);
    });
    it('endCall_187', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000B07)).assertEqual(0x00100000814);
    });
    it('endCall_188', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000B08)).assertEqual(0x00100000815);
    });
    it('endCall_189', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000B09)).assertEqual(0x00100000816);
    });
    it('endCall_190', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000B0A)).assertEqual(0x00100000817);
    });
    it('endCall_191', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000B0B)).assertEqual(0x00100000818);
    });
    it('endCall_192', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000B0F)).assertEqual(0x00200000000);
    });
    it('endCall_193', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000B12)).assertEqual(0x0000000003F);
    });
    it('endCall_194', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000811)).assertEqual(0x00000000040);
    });
    it('endCall_195', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000006)).assertEqual(0x00100001007);
    });
    it('endCall_196', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000017)).assertEqual(0x00100000A11);
    });
    it('endCall_197', Level.LEVEL1, () => {
      expect(KeyboardMap.toLogicalKey.get(0x00000000004)).assertEqual(0x00100001004);
    });
  });
  describe('KeyboardMap.toPhysicalKey', () => {
    it('audioVolumeMute_1', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x000000007D0)).assertEqual(0x00000070027);
    });
    it('audioVolumeMute_2', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x000000007D1)).assertEqual(0x0000007001E);
    });
    it('audioVolumeMute_3', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x000000007D2)).assertEqual(0x0000007001F);
    });
    it('audioVolumeMute_4', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x000000007D3)).assertEqual(0x00000070020);
    });
    it('audioVolumeMute_5', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x000000007D4)).assertEqual(0x00000070021);
    });
    it('audioVolumeMute_6', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x000000007D5)).assertEqual(0x00000070022);
    });
    it('audioVolumeMute_7', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x000000007D6)).assertEqual(0x00000070023);
    });
    it('audioVolumeMute_8', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x000000007D7)).assertEqual(0x00000070024);
    });
    it('audioVolumeMute_9', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x000000007D8)).assertEqual(0x00000070025);
    });
    it('audioVolumeMute_10', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x000000007D9)).assertEqual(0x00000070026);
    });
    it('audioVolumeMute_11', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x000000007DC)).assertEqual(0x00000070052);
    });
    it('audioVolumeMute_12', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x000000007DD)).assertEqual(0x00000070051);
    });
    it('audioVolumeMute_13', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x000000007DE)).assertEqual(0x00000070050);
    });
    it('audioVolumeMute_14', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x000000007DF)).assertEqual(0x0000007004F);
    });
    it('audioVolumeMute_15', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x000000007E1)).assertEqual(0x00000070004);
    });
    it('audioVolumeMute_16', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x000000007E2)).assertEqual(0x00000070005);
    });
    it('audioVolumeMute_17', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x000000007E3)).assertEqual(0x00000070006);
    });
    it('audioVolumeMute_18', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x000000007E4)).assertEqual(0x00000070007);
    });
    it('audioVolumeMute_19', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x000000007E5)).assertEqual(0x00000070008);
    });
    it('audioVolumeMute_20', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x000000007E6)).assertEqual(0x00000070009);
    });
    it('audioVolumeMute_21', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x000000007E7)).assertEqual(0x0000007000A);
    });
    it('audioVolumeMute_22', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x000000007E8)).assertEqual(0x0000007000B);
    });
    it('audioVolumeMute_23', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x000000007E9)).assertEqual(0x0000007000C);
    });
    it('audioVolumeMute_24', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x000000007EA)).assertEqual(0x0000007000D);
    });
    it('audioVolumeMute_25', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x000000007EB)).assertEqual(0x0000007000E);
    });
    it('audioVolumeMute_26', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x000000007EC)).assertEqual(0x0000007000F);
    });
    it('audioVolumeMute_27', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x000000007ED)).assertEqual(0x00000070010);
    });
    it('audioVolumeMute_28', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x000000007EE)).assertEqual(0x00000070011);
    });
    it('audioVolumeMute_29', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x000000007EF)).assertEqual(0x00000070012);
    });
    it('audioVolumeMute_30', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x000000007F0)).assertEqual(0x00000070013);
    });
    it('audioVolumeMute_31', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x000000007F1)).assertEqual(0x00000070014);
    });
    it('audioVolumeMute_32', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x000000007F2)).assertEqual(0x00000070015);
    });
    it('audioVolumeMute_33', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x000000007F3)).assertEqual(0x00000070016);
    });
    it('audioVolumeMute_34', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x000000007F4)).assertEqual(0x00000070017);
    });
    it('audioVolumeMute_35', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x000000007F5)).assertEqual(0x00000070018);
    });
    it('audioVolumeMute_36', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x000000007F6)).assertEqual(0x00000070019);
    });
    it('audioVolumeMute_37', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x000000007F7)).assertEqual(0x0000007001A);
    });
    it('audioVolumeMute_38', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x000000007F8)).assertEqual(0x0000007001B);
    });
    it('audioVolumeMute_39', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x000000007F9)).assertEqual(0x0000007001C);
    });
    it('audioVolumeMute_40', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x000000007FA)).assertEqual(0x0000007001D);
    });
    it('audioVolumeMute_41', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x000000007FB)).assertEqual(0x00000070036);
    });
    it('audioVolumeMute_42', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x000000007FC)).assertEqual(0x00000070037);
    });
    it('audioVolumeMute_43', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x000000007FD)).assertEqual(0x000000700E2);
    });
    it('audioVolumeMute_44', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x000000007FE)).assertEqual(0x000000700E6);
    });
    it('audioVolumeMute_45', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x000000007FF)).assertEqual(0x000000700E1);
    });
    it('audioVolumeMute_46', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x00000000800)).assertEqual(0x000000700E5);
    });
    it('audioVolumeMute_47', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x00000000801)).assertEqual(0x0000007002B);
    });
    it('audioVolumeMute_48', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x00000000802)).assertEqual(0x0000007002C);
    });
    it('audioVolumeMute_49', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x00000000805)).assertEqual(0x000000C018A);
    });
    it('audioVolumeMute_50', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x00000000806)).assertEqual(0x00000070028);
    });
    it('audioVolumeMute_51', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x00000000807)).assertEqual(0x0000007002A);
    });
    it('audioVolumeMute_52', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x00000000808)).assertEqual(0x00000070035);
    });
    it('audioVolumeMute_53', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x00000000809)).assertEqual(0x0000007002D);
    });
    it('audioVolumeMute_54', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x0000000080A)).assertEqual(0x0000007002E);
    });
    it('audioVolumeMute_55', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x0000000080B)).assertEqual(0x0000007002F);
    });
    it('audioVolumeMute_56', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x0000000080C)).assertEqual(0x00000070030);
    });
    it('audioVolumeMute_57', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x0000000080D)).assertEqual(0x00000070031);
    });
    it('audioVolumeMute_58', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x0000000080E)).assertEqual(0x00000070033);
    });
    it('audioVolumeMute_59', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x0000000080F)).assertEqual(0x00000070034);
    });
    it('audioVolumeMute_60', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x00000000810)).assertEqual(0x00000070038);
    });
    it('audioVolumeMute_61', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x00000000813)).assertEqual(0x00000070065);
    });
    it('audioVolumeMute_62', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x00000000814)).assertEqual(0x0000007004B);
    });
    it('audioVolumeMute_63', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x00000000815)).assertEqual(0x0000007004E);
    });
    it('audioVolumeMute_64', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x00000000816)).assertEqual(0x00000070029);
    });
    it('audioVolumeMute_65', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x00000000817)).assertEqual(0x0000007004C);
    });
    it('audioVolumeMute_66', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x00000000818)).assertEqual(0x000000700E0);
    });
    it('audioVolumeMute_67', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x00000000819)).assertEqual(0x000000700E4);
    });
    it('audioVolumeMute_68', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x0000000081A)).assertEqual(0x00000070039);
    });
    it('audioVolumeMute_69', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x0000000081B)).assertEqual(0x00000070047);
    });
    it('audioVolumeMute_70', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x0000000081C)).assertEqual(0x000000700E3);
    });
    it('audioVolumeMute_71', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x0000000081D)).assertEqual(0x000000700E7);
    });
    it('audioVolumeMute_72', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x0000000081E)).assertEqual(0x00000000012);
    });
    it('audioVolumeMute_73', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x0000000081F)).assertEqual(0x00000070046);
    });
    it('audioVolumeMute_74', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x00000000820)).assertEqual(0x00000070048);
    });
    it('audioVolumeMute_75', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x00000000821)).assertEqual(0x0000007004A);
    });
    it('audioVolumeMute_76', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x00000000822)).assertEqual(0x0000007004D);
    });
    it('audioVolumeMute_77', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x00000000823)).assertEqual(0x00000070049);
    });
    it('audioVolumeMute_78', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x00000000824)).assertEqual(0x000000C0225);
    });
    it('audioVolumeMute_79', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x00000000825)).assertEqual(0x000000C00B0);
    });
    it('audioVolumeMute_80', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x00000000826)).assertEqual(0x000000C00B1);
    });
    it('audioVolumeMute_81', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x00000000828)).assertEqual(0x000000C00B8);
    });
    it('audioVolumeMute_82', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x00000000829)).assertEqual(0x000000C00B2);
    });
    it('audioVolumeMute_83', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x0000000082A)).assertEqual(0x0000007003A);
    });
    it('audioVolumeMute_84', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x0000000082B)).assertEqual(0x0000007003B);
    });
    it('audioVolumeMute_85', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x0000000082C)).assertEqual(0x0000007003C);
    });
    it('audioVolumeMute_86', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x0000000082D)).assertEqual(0x0000007003D);
    });
    it('audioVolumeMute_87', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x0000000082E)).assertEqual(0x0000007003E);
    });
    it('audioVolumeMute_88', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x0000000082F)).assertEqual(0x0000007003F);
    });
    it('audioVolumeMute_89', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x00000000830)).assertEqual(0x00000070040);
    });
    it('audioVolumeMute_90', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x00000000831)).assertEqual(0x00000070041);
    });
    it('audioVolumeMute_91', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x00000000832)).assertEqual(0x00000070042);
    });
    it('audioVolumeMute_92', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x00000000833)).assertEqual(0x00000070043);
    });
    it('audioVolumeMute_93', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x00000000834)).assertEqual(0x00000070044);
    });
    it('audioVolumeMute_94', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x00000000835)).assertEqual(0x00000070045);
    });
    it('audioVolumeMute_95', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x00000000836)).assertEqual(0x00000070053);
    });
    it('audioVolumeMute_96', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x00000000837)).assertEqual(0x00000070062);
    });
    it('audioVolumeMute_97', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x00000000838)).assertEqual(0x00000070059);
    });
    it('audioVolumeMute_98', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x00000000839)).assertEqual(0x0000007005A);
    });
    it('audioVolumeMute_99', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x0000000083A)).assertEqual(0x0000007005B);
    });
    it('audioVolumeMute_100', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x0000000083B)).assertEqual(0x0000007005C);
    });
    it('audioVolumeMute_101', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x0000000083C)).assertEqual(0x0000007005D);
    });
    it('audioVolumeMute_102', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x0000000083D)).assertEqual(0x0000007005E);
    });
    it('audioVolumeMute_103', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x0000000083E)).assertEqual(0x0000007005F);
    });
    it('audioVolumeMute_104', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x0000000083F)).assertEqual(0x00000070060);
    });
    it('audioVolumeMute_105', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x00000000840)).assertEqual(0x00000070061);
    });
    it('audioVolumeMute_106', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x00000000841)).assertEqual(0x00000070054);
    });
    it('audioVolumeMute_107', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x00000000842)).assertEqual(0x00000070055);
    });
    it('audioVolumeMute_108', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x00000000843)).assertEqual(0x00000070056);
    });
    it('audioVolumeMute_109', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x00000000844)).assertEqual(0x00000070057);
    });
    it('audioVolumeMute_110', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x00000000845)).assertEqual(0x00000070063);
    });
    it('audioVolumeMute_111', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x00000000846)).assertEqual(0x00000070085);
    });
    it('audioVolumeMute_112', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x00000000847)).assertEqual(0x00000070058);
    });
    it('audioVolumeMute_113', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x00000000848)).assertEqual(0x00000070067);
    });
    it('audioVolumeMute_114', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x00000000849)).assertEqual(0x000000700B6);
    });
    it('audioVolumeMute_115', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x0000000084A)).assertEqual(0x000000700B7);
    });
    it('audioVolumeMute_116', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x00000000010)).assertEqual(0x00000070080);
    });
    it('audioVolumeMute_117', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x00000000011)).assertEqual(0x00000070081);
    });
    it('audioVolumeMute_118', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x00000000012)).assertEqual(0x00000070066);
    });
    it('audioVolumeMute_119', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x00000000001)).assertEqual(0x0000007004A);
    });
    it('audioVolumeMute_120', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x00000000028)).assertEqual(0x000000C006F);
    });
    it('audioVolumeMute_121', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x00000000029)).assertEqual(0x000000C0070);
    });
    it('audioVolumeMute_122', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x0000000000A)).assertEqual(0x000000C00CD);
    });
    it('audioVolumeMute_123', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x0000000000B)).assertEqual(0x000000C00B7);
    });
    it('audioVolumeMute_124', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x0000000000C)).assertEqual(0x000000C00B5);
    });
    it('audioVolumeMute_125', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x0000000000D)).assertEqual(0x000000C00B6);
    });
    it('audioVolumeMute_126', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x0000000000E)).assertEqual(0x000000C00B4);
    });
    it('audioVolumeMute_127', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x0000000000F)).assertEqual(0x000000C00B3);
    });
    it('audioVolumeMute_128', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x00000000A28)).assertEqual(0x00000010082);
    });
    it('audioVolumeMute_129', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x00000000A2E)).assertEqual(0x0000007008A);
    });
    it('audioVolumeMute_130', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x00000000A30)).assertEqual(0x0000007008B);
    });
    it('audioVolumeMute_131', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x00000000A37)).assertEqual(0x00000070089);
    });
    it('audioVolumeMute_132', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x00000000A39)).assertEqual(0x00000070079);
    });
    it('audioVolumeMute_133', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x00000000A3A)).assertEqual(0x000000700A3);
    });
    it('audioVolumeMute_134', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x00000000A3B)).assertEqual(0x0000007007A);
    });
    it('audioVolumeMute_135', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x00000000A3C)).assertEqual(0x0000007007C);
    });
    it('audioVolumeMute_136', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x00000000A3D)).assertEqual(0x00000070074);
    });
    it('audioVolumeMute_137', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x00000000A3E)).assertEqual(0x0000007007D);
    });
    it('audioVolumeMute_138', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x00000000A3F)).assertEqual(0x0000007007E);
    });
    it('audioVolumeMute_139', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x00000000A40)).assertEqual(0x0000007007B);
    });
    it('audioVolumeMute_140', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x00000000A41)).assertEqual(0x00000070075);
    });
    it('audioVolumeMute_141', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x00000000A44)).assertEqual(0x000000C022A);
    });
    it('audioVolumeMute_142', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x00000000A46)).assertEqual(0x000000C00CD);
    });
    it('audioVolumeMute_143', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x00000000A48)).assertEqual(0x000000C0203);
    });
    it('audioVolumeMute_144', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x00000000A4B)).assertEqual(0x000000C0227);
    });
    it('audioVolumeMute_145', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x00000000A4C)).assertEqual(0x000000C0094);
    });
    it('audioVolumeMute_146', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x00000000A51)).assertEqual(0x000000C0279);
    });
    it('audioVolumeMute_147', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x00000000A52)).assertEqual(0x000000C0203);
    });
    it('audioVolumeMute_148', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x00000000A55)).assertEqual(0x000000C0208);
    });
    it('audioVolumeMute_149', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x00000000A5F)).assertEqual(0x000000C0207);
    });
    it('audioVolumeMute_150', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x00000000A68)).assertEqual(0x000000C0060);
    });
    it('audioVolumeMute_151', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x00000000A82)).assertEqual(0x000000C009C);
    });
    it('audioVolumeMute_152', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x00000000A83)).assertEqual(0x000000C009D);
    });
    it('audioVolumeMute_153', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x00000000A8A)).assertEqual(0x000000C022D);
    });
    it('audioVolumeMute_154', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x00000000A8B)).assertEqual(0x000000C022E);
    });
    it('audioVolumeMute_155', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x00000000A98)).assertEqual(0x000000C01AB);
    });
    it('audioVolumeMute_156', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x00000000AF2)).assertEqual(0x00000010083);
    });
    it('audioVolumeMute_157', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x00000000AFD)).assertEqual(0x000000C00B8);
    });
    it('audioVolumeMute_158', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x00000000B00)).assertEqual(0x00000070068);
    });
    it('audioVolumeMute_159', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x00000000B01)).assertEqual(0x00000070069);
    });
    it('audioVolumeMute_160', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x00000000B02)).assertEqual(0x0000007006A);
    });
    it('audioVolumeMute_161', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x00000000B03)).assertEqual(0x0000007006B);
    });
    it('audioVolumeMute_162', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x00000000B04)).assertEqual(0x0000007006C);
    });
    it('audioVolumeMute_163', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x00000000B05)).assertEqual(0x0000007006D);
    });
    it('audioVolumeMute_164', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x00000000B06)).assertEqual(0x0000007006E);
    });
    it('audioVolumeMute_165', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x00000000B07)).assertEqual(0x0000007006F);
    });
    it('audioVolumeMute_166', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x00000000B08)).assertEqual(0x00000070070);
    });
    it('audioVolumeMute_167', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x00000000B09)).assertEqual(0x00000070071);
    });
    it('audioVolumeMute_168', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x00000000B0A)).assertEqual(0x00000070072);
    });
    it('audioVolumeMute_169', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x00000000B0B)).assertEqual(0x00000070073);
    });
    it('audioVolumeMute_170', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x00000000B0F)).assertEqual(0x00000000014);
    });
    it('audioVolumeMute_171', Level.LEVEL1, () => {
      expect(KeyboardMap.toPhysicalKey.get(0x00000000017)).assertEqual(0x0000007007F);
    });
  });
  describe('KeyboardMap.ohKeyToScanCode', () => {
    it('audioVolumeMute_1', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x000000007D0)).assertEqual(0x0000000000B);
    });
    it('audioVolumeMute_2', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x000000007D1)).assertEqual(0x00000000002);
    });
    it('audioVolumeMute_3', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x000000007D2)).assertEqual(0x00000000003);
    });
    it('audioVolumeMute_4', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x000000007D3)).assertEqual(0x00000000004);
    });
    it('audioVolumeMute_5', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x000000007D4)).assertEqual(0x00000000005);
    });
    it('audioVolumeMute_6', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x000000007D5)).assertEqual(0x00000000006);
    });
    it('audioVolumeMute_7', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x000000007D6)).assertEqual(0x00000000007);
    });
    it('audioVolumeMute_8', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x000000007D7)).assertEqual(0x00000000008);
    });
    it('audioVolumeMute_9', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x000000007D8)).assertEqual(0x00000000009);
    });
    it('audioVolumeMute_10', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x000000007D9)).assertEqual(0x0000000000A);
    });
    it('audioVolumeMute_11', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x000000007DC)).assertEqual(0x00000000067);
    });
    it('audioVolumeMute_12', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x000000007DD)).assertEqual(0x0000000006C);
    });
    it('audioVolumeMute_13', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x000000007DE)).assertEqual(0x00000000069);
    });
    it('audioVolumeMute_14', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x000000007DF)).assertEqual(0x0000000006A);
    });
    it('audioVolumeMute_15', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x000000007E1)).assertEqual(0x0000000001E);
    });
    it('audioVolumeMute_16', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x000000007E2)).assertEqual(0x00000000030);
    });
    it('audioVolumeMute_17', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x000000007E3)).assertEqual(0x0000000002E);
    });
    it('audioVolumeMute_18', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x000000007E4)).assertEqual(0x00000000020);
    });
    it('audioVolumeMute_19', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x000000007E5)).assertEqual(0x00000000012);
    });
    it('audioVolumeMute_20', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x000000007E6)).assertEqual(0x00000000021);
    });
    it('audioVolumeMute_21', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x000000007E7)).assertEqual(0x00000000022);
    });
    it('audioVolumeMute_22', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x000000007E8)).assertEqual(0x00000000023);
    });
    it('audioVolumeMute_23', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x000000007E9)).assertEqual(0x00000000017);
    });
    it('audioVolumeMute_24', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x000000007EA)).assertEqual(0x00000000024);
    });
    it('audioVolumeMute_25', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x000000007EB)).assertEqual(0x00000000025);
    });
    it('audioVolumeMute_26', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x000000007EC)).assertEqual(0x00000000026);
    });
    it('audioVolumeMute_27', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x000000007ED)).assertEqual(0x00000000032);
    });
    it('audioVolumeMute_28', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x000000007EE)).assertEqual(0x00000000031);
    });
    it('audioVolumeMute_29', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x000000007EF)).assertEqual(0x00000000018);
    });
    it('audioVolumeMute_30', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x000000007F0)).assertEqual(0x00000000019);
    });
    it('audioVolumeMute_31', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x000000007F1)).assertEqual(0x00000000010);
    });
    it('audioVolumeMute_32', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x000000007F2)).assertEqual(0x00000000013);
    });
    it('audioVolumeMute_33', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x000000007F3)).assertEqual(0x0000000001F);
    });
    it('audioVolumeMute_34', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x000000007F4)).assertEqual(0x00000000014);
    });
    it('audioVolumeMute_35', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x000000007F5)).assertEqual(0x00000000016);
    });
    it('audioVolumeMute_36', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x000000007F6)).assertEqual(0x0000000002F);
    });
    it('audioVolumeMute_37', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x000000007F7)).assertEqual(0x00000000011);
    });
    it('audioVolumeMute_38', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x000000007F8)).assertEqual(0x0000000002D);
    });
    it('audioVolumeMute_39', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x000000007F9)).assertEqual(0x00000000015);
    });
    it('audioVolumeMute_40', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x000000007FA)).assertEqual(0x0000000002C);
    });
    it('audioVolumeMute_41', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x000000007FB)).assertEqual(0x00000000033);
    });
    it('audioVolumeMute_42', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x000000007FC)).assertEqual(0x00000000034);
    });
    it('audioVolumeMute_43', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x000000007FD)).assertEqual(0x00000000038);
    });
    it('audioVolumeMute_44', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x000000007FE)).assertEqual(0x00000000064);
    });
    it('audioVolumeMute_45', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x000000007FF)).assertEqual(0x0000000002A);
    });
    it('audioVolumeMute_46', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x00000000800)).assertEqual(0x00000000036);
    });
    it('audioVolumeMute_47', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x00000000801)).assertEqual(0x0000000000F);
    });
    it('audioVolumeMute_48', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x00000000802)).assertEqual(0x00000000039);
    });
    it('audioVolumeMute_49', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x00000000805)).assertEqual(0x000000000D7);
    });
    it('audioVolumeMute_50', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x00000000806)).assertEqual(0x0000000001C);
    });
    it('audioVolumeMute_51', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x00000000807)).assertEqual(0x0000000000E);
    });
    it('audioVolumeMute_52', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x00000000808)).assertEqual(0x00000000029);
    });
    it('audioVolumeMute_53', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x00000000809)).assertEqual(0x0000000000C);
    });
    it('audioVolumeMute_54', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x0000000080A)).assertEqual(0x0000000000D);
    });
    it('audioVolumeMute_55', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x0000000080B)).assertEqual(0x0000000001A);
    });
    it('audioVolumeMute_56', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x0000000080C)).assertEqual(0x0000000001B);
    });
    it('audioVolumeMute_57', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x0000000080D)).assertEqual(0x00000000056);
    });
    it('audioVolumeMute_58', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x0000000080E)).assertEqual(0x00000000027);
    });
    it('audioVolumeMute_59', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x0000000080F)).assertEqual(0x00000000028);
    });
    it('audioVolumeMute_60', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x00000000810)).assertEqual(0x00000000035);
    });
    it('audioVolumeMute_61', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x00000000813)).assertEqual(0x0000000008B);
    });
    it('audioVolumeMute_62', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x00000000814)).assertEqual(0x000000000B1);
    });
    it('audioVolumeMute_63', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x00000000815)).assertEqual(0x000000000B2);
    });
    it('audioVolumeMute_64', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x00000000816)).assertEqual(0x00000000001);
    });
    it('audioVolumeMute_65', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x00000000817)).assertEqual(0x0000000006F);
    });
    it('audioVolumeMute_66', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x00000000818)).assertEqual(0x0000000001D);
    });
    it('audioVolumeMute_67', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x00000000819)).assertEqual(0x00000000061);
    });
    it('audioVolumeMute_68', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x0000000081A)).assertEqual(0x0000000003A);
    });
    it('audioVolumeMute_69', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x0000000081B)).assertEqual(0x00000000046);
    });
    it('audioVolumeMute_70', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x0000000081C)).assertEqual(0x0000000007D);
    });
    it('audioVolumeMute_71', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x0000000081D)).assertEqual(0x0000000007E);
    });
    it('audioVolumeMute_72', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x0000000081E)).assertEqual(0x000000001D0);
    });
    it('audioVolumeMute_73', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x0000000081F)).assertEqual(0x00000000063);
    });
    it('audioVolumeMute_74', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x00000000820)).assertEqual(0x0000000019B);
    });
    it('audioVolumeMute_75', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x00000000821)).assertEqual(0x00000000066);
    });
    it('audioVolumeMute_76', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x00000000822)).assertEqual(0x0000000006B);
    });
    it('audioVolumeMute_77', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x00000000823)).assertEqual(0x0000000006E);
    });
    it('audioVolumeMute_78', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x00000000824)).assertEqual(0x0000000009F);
    });
    it('audioVolumeMute_79', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x00000000825)).assertEqual(0x000000000CF);
    });
    it('audioVolumeMute_80', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x00000000826)).assertEqual(0x000000000C9);
    });
    it('audioVolumeMute_81', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x00000000828)).assertEqual(0x000000000A2);
    });
    it('audioVolumeMute_82', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x00000000829)).assertEqual(0x000000000A7);
    });
    it('audioVolumeMute_83', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x0000000082A)).assertEqual(0x0000000003B);
    });
    it('audioVolumeMute_84', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x0000000082B)).assertEqual(0x0000000003C);
    });
    it('audioVolumeMute_85', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x0000000082C)).assertEqual(0x0000000003D);
    });
    it('audioVolumeMute_86', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x0000000082D)).assertEqual(0x0000000003E);
    });
    it('audioVolumeMute_87', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x0000000082E)).assertEqual(0x0000000003F);
    });
    it('audioVolumeMute_88', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x0000000082F)).assertEqual(0x00000000040);
    });
    it('audioVolumeMute_89', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x00000000830)).assertEqual(0x00000000041);
    });
    it('audioVolumeMute_90', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x00000000831)).assertEqual(0x00000000042);
    });
    it('audioVolumeMute_91', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x00000000832)).assertEqual(0x00000000043);
    });
    it('audioVolumeMute_92', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x00000000833)).assertEqual(0x00000000044);
    });
    it('audioVolumeMute_93', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x00000000834)).assertEqual(0x00000000057);
    });
    it('audioVolumeMute_94', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x00000000835)).assertEqual(0x00000000058);
    });
    it('audioVolumeMute_95', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x00000000836)).assertEqual(0x00000000045);
    });
    it('audioVolumeMute_96', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x00000000837)).assertEqual(0x00000000052);
    });
    it('audioVolumeMute_97', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x00000000838)).assertEqual(0x0000000004F);
    });
    it('audioVolumeMute_98', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x00000000839)).assertEqual(0x00000000050);
    });
    it('audioVolumeMute_99', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x0000000083A)).assertEqual(0x00000000051);
    });
    it('audioVolumeMute_100', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x0000000083B)).assertEqual(0x0000000004B);
    });
    it('audioVolumeMute_101', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x0000000083C)).assertEqual(0x0000000004C);
    });
    it('audioVolumeMute_102', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x0000000083D)).assertEqual(0x0000000004D);
    });
    it('audioVolumeMute_103', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x0000000083E)).assertEqual(0x00000000047);
    });
    it('audioVolumeMute_104', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x0000000083F)).assertEqual(0x00000000048);
    });
    it('audioVolumeMute_105', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x00000000840)).assertEqual(0x00000000049);
    });
    it('audioVolumeMute_106', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x00000000841)).assertEqual(0x00000000062);
    });
    it('audioVolumeMute_107', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x00000000842)).assertEqual(0x00000000037);
    });
    it('audioVolumeMute_108', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x00000000843)).assertEqual(0x0000000004A);
    });
    it('audioVolumeMute_109', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x00000000844)).assertEqual(0x0000000004E);
    });
    it('audioVolumeMute_110', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x00000000845)).assertEqual(0x00000000053);
    });
    it('audioVolumeMute_111', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x00000000846)).assertEqual(0x00000000079);
    });
    it('audioVolumeMute_112', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x00000000847)).assertEqual(0x00000000060);
    });
    it('audioVolumeMute_113', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x00000000848)).assertEqual(0x00000000075);
    });
    it('audioVolumeMute_114', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x00000000849)).assertEqual(0x000000000B3);
    });
    it('audioVolumeMute_115', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x0000000084A)).assertEqual(0x000000000B4);
    });
    it('audioVolumeMute_116', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x00000000010)).assertEqual(0x00000000073);
    });
    it('audioVolumeMute_117', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x00000000011)).assertEqual(0x00000000072);
    });
    it('audioVolumeMute_118', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x00000000012)).assertEqual(0x00000000098);
    });
    it('audioVolumeMute_119', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x00000000001)).assertEqual(0x00000000066);
    });
    it('audioVolumeMute_120', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x00000000028)).assertEqual(0x000000000E1);
    });
    it('audioVolumeMute_121', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x00000000029)).assertEqual(0x000000000E0);
    });
    it('audioVolumeMute_122', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x0000000000A)).assertEqual(0x000000000A4);
    });
    it('audioVolumeMute_123', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x0000000000B)).assertEqual(0x000000000A6);
    });
    it('audioVolumeMute_124', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x0000000000C)).assertEqual(0x000000000A3);
    });
    it('audioVolumeMute_125', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x0000000000D)).assertEqual(0x000000000A5);
    });
    it('audioVolumeMute_126', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x0000000000E)).assertEqual(0x000000000A8);
    });
    it('audioVolumeMute_127', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x0000000000F)).assertEqual(0x000000000D0);
    });
    it('audioVolumeMute_128', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x00000000A28)).assertEqual(0x0000000008E);
    });
    it('audioVolumeMute_129', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x00000000A2E)).assertEqual(0x0000000005C);
    });
    it('audioVolumeMute_130', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x00000000A30)).assertEqual(0x0000000005E);
    });
    it('audioVolumeMute_131', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x00000000A37)).assertEqual(0x0000000007C);
    });
    it('audioVolumeMute_132', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x00000000A39)).assertEqual(0x00000000081);
    });
    it('audioVolumeMute_133', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x00000000A3A)).assertEqual(0x00000000082);
    });
    it('audioVolumeMute_134', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x00000000A3B)).assertEqual(0x00000000083);
    });
    it('audioVolumeMute_135', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x00000000A3C)).assertEqual(0x00000000085);
    });
    it('audioVolumeMute_136', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x00000000A3D)).assertEqual(0x00000000086);
    });
    it('audioVolumeMute_137', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x00000000A3E)).assertEqual(0x00000000087);
    });
    it('audioVolumeMute_138', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x00000000A3F)).assertEqual(0x00000000088);
    });
    it('audioVolumeMute_139', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x00000000A40)).assertEqual(0x00000000089);
    });
    it('audioVolumeMute_140', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x00000000A41)).assertEqual(0x0000000008A);
    });
    it('audioVolumeMute_141', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x00000000A44)).assertEqual(0x0000000009C);
    });
    it('audioVolumeMute_142', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x00000000A46)).assertEqual(0x000000000A4);
    });
    it('audioVolumeMute_143', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x00000000A48)).assertEqual(0x000000000CE);
    });
    it('audioVolumeMute_144', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x00000000A4C)).assertEqual(0x000000000AE);
    });
    it('audioVolumeMute_145', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x00000000A51)).assertEqual(0x000000000B6);
    });
    it('audioVolumeMute_146', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x00000000A52)).assertEqual(0x000000000CE);
    });
    it('audioVolumeMute_147', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x00000000A55)).assertEqual(0x000000000D2);
    });
    it('audioVolumeMute_148', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x00000000A68)).assertEqual(0x00000000166);
    });
    it('audioVolumeMute_149', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x00000000A82)).assertEqual(0x00000000192);
    });
    it('audioVolumeMute_150', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x00000000A83)).assertEqual(0x00000000193);
    });
    it('audioVolumeMute_151', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x00000000AF2)).assertEqual(0x0000000008F);
    });
    it('audioVolumeMute_152', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x00000000AFD)).assertEqual(0x000000000A2);
    });
    it('audioVolumeMute_153', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x00000000B00)).assertEqual(0x000000000B7);
    });
    it('audioVolumeMute_154', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x00000000B01)).assertEqual(0x000000000B8);
    });
    it('audioVolumeMute_155', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x00000000B02)).assertEqual(0x000000000B9);
    });
    it('audioVolumeMute_156', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x00000000B03)).assertEqual(0x000000000BA);
    });
    it('audioVolumeMute_157', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x00000000B04)).assertEqual(0x000000000BB);
    });
    it('audioVolumeMute_158', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x00000000B05)).assertEqual(0x000000000BC);
    });
    it('audioVolumeMute_159', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x00000000B06)).assertEqual(0x000000000BD);
    });
    it('audioVolumeMute_160', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x00000000B07)).assertEqual(0x000000000BE);
    });
    it('audioVolumeMute_161', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x00000000B08)).assertEqual(0x000000000BF);
    });
    it('audioVolumeMute_162', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x00000000B09)).assertEqual(0x000000000C0);
    });
    it('audioVolumeMute_163', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x00000000B0A)).assertEqual(0x000000000C1);
    });
    it('audioVolumeMute_164', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x00000000B0B)).assertEqual(0x000000000C2);
    });
    it('audioVolumeMute_165', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x00000000B0F)).assertEqual(0x000000000CD);
    });
    it('audioVolumeMute_166', Level.LEVEL1, () => {
      expect(KeyboardMap.ohKeyToScanCode.get(0x00000000017)).assertEqual(0x00000000071);
    });
  });
  describe('KeyboardMap.constants', () => {
    it('kValueMask', Level.LEVEL0, () => { expect(KeyboardMap.kValueMask).assertEqual(0x000FFFFFFFF); });
    it('kUnicodePlane', Level.LEVEL0, () => { expect(KeyboardMap.kUnicodePlane).assertEqual(0x00000000000); });
    it('kOhosPlane', Level.LEVEL0, () => { expect(KeyboardMap.kOhosPlane).assertEqual(0x01900000000); });
  });
  describe('KeyboardMap.modifierGoals', () => {
    it('has3', Level.LEVEL0, () => { expect(KeyboardMap.modifierGoals.length).assertEqual(3); });
    it('ctrl', Level.LEVEL0, () => { expect(KeyboardMap.modifierGoals[0].name).assertEqual('Ctrl'); expect(KeyboardMap.modifierGoals[0].keys.length).assertEqual(2); });
    it('shift', Level.LEVEL0, () => { expect(KeyboardMap.modifierGoals[1].name).assertEqual('Shift'); expect(KeyboardMap.modifierGoals[1].keys.length).assertEqual(2); });
    it('alt', Level.LEVEL0, () => { expect(KeyboardMap.modifierGoals[2].name).assertEqual('Alt'); expect(KeyboardMap.modifierGoals[2].keys.length).assertEqual(2); });
  });
  describe('ModifierKeyMetaInfo.constants', () => {
    it('NONE', Level.LEVEL0, () => { expect(ModifierKeyMetaInfo.NONE).assertEqual(0); });
    it('ALT', Level.LEVEL0, () => { expect(ModifierKeyMetaInfo.ALT).assertEqual(0x02); });
    it('ALT_LEFT', Level.LEVEL0, () => { expect(ModifierKeyMetaInfo.ALT_LEFT).assertEqual(0x10); });
    it('ALT_RIGHT', Level.LEVEL0, () => { expect(ModifierKeyMetaInfo.ALT_RIGHT).assertEqual(0x20); });
    it('SHIFT', Level.LEVEL0, () => { expect(ModifierKeyMetaInfo.SHIFT).assertEqual(0x01); });
    it('SHIFT_LEFT', Level.LEVEL0, () => { expect(ModifierKeyMetaInfo.SHIFT_LEFT).assertEqual(0x40); });
    it('SHIFT_RIGHT', Level.LEVEL0, () => { expect(ModifierKeyMetaInfo.SHIFT_RIGHT).assertEqual(0x80); });
    it('CTRL', Level.LEVEL0, () => { expect(ModifierKeyMetaInfo.CTRL).assertEqual(0x1000); });
    it('CTRL_LEFT', Level.LEVEL0, () => { expect(ModifierKeyMetaInfo.CTRL_LEFT).assertEqual(0x2000); });
    it('CTRL_RIGHT', Level.LEVEL0, () => { expect(ModifierKeyMetaInfo.CTRL_RIGHT).assertEqual(0x4000); });
    it('META', Level.LEVEL0, () => { expect(ModifierKeyMetaInfo.META).assertEqual(0x10000); });
    it('META_LEFT', Level.LEVEL0, () => { expect(ModifierKeyMetaInfo.META_LEFT).assertEqual(0x20000); });
    it('META_RIGHT', Level.LEVEL0, () => { expect(ModifierKeyMetaInfo.META_RIGHT).assertEqual(0x40000); });
  });
}