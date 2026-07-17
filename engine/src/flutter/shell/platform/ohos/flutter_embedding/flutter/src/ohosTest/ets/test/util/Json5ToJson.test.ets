import { describe, it, expect, Level } from '@ohos/hypium';
import { json5Tojson } from '../../../../main/ets/util/Json5ToJson';

export default function json5ToJsonTest() {
  describe('Json5ToJson.lineComments', () => {
    it('removes line comment before newline', Level.LEVEL0, () => {
      expect(json5Tojson('// comment\nvalue')).assertEqual('\nvalue');
    });

    it('removes line comment after code', Level.LEVEL1, () => {
      expect(json5Tojson('value // comment')).assertEqual('value ');
    });

    it('removes line comment without newline at end', Level.LEVEL1, () => {
      expect(json5Tojson('// no newline')).assertEqual('');
    });

    it('removes multiple line comments', Level.LEVEL1, () => {
      expect(json5Tojson('line1 // c1\nline2 // c2')).assertEqual('line1 \nline2 ');
    });

    it('removes standalone double-slash', Level.LEVEL1, () => {
      expect(json5Tojson('//')).assertEqual('');
    });
  });

  describe('Json5ToJson.blockComments', () => {
    it('removes block comment before content', Level.LEVEL0, () => {
      expect(json5Tojson('/* comment */value')).assertEqual('value');
    });

    it('removes block comment after content', Level.LEVEL1, () => {
      expect(json5Tojson('value/* comment */')).assertEqual('value');
    });

    it('removes block comment in middle of content', Level.LEVEL1, () => {
      expect(json5Tojson('val/* c */ue')).assertEqual('value');
    });

    it('removes multiline block comment', Level.LEVEL1, () => {
      expect(json5Tojson('/* line1\nline2 */value')).assertEqual('value');
    });

    it('removes empty block comment', Level.LEVEL1, () => {
      expect(json5Tojson('/**/')).assertEqual('');
    });

    it('removes unclosed block comment to end', Level.LEVEL1, () => {
      expect(json5Tojson('/* unclosed')).assertEqual('');
    });

    it('removes two separate block comments', Level.LEVEL1, () => {
      expect(json5Tojson('/* outer */ value /* inner */')).assertEqual(' value ');
    });
  });

  describe('Json5ToJson.stringPreservation', () => {
    it('preserves double-slash inside double-quoted string', Level.LEVEL0, () => {
      expect(json5Tojson('"// not a comment"')).assertEqual('"// not a comment"');
    });

    it('preserves block comment markers inside double-quoted string', Level.LEVEL1, () => {
      expect(json5Tojson('"/* not a comment */"')).assertEqual('"/* not a comment */"');
    });

    it('preserves double-slash inside single-quoted string', Level.LEVEL1, () => {
      expect(json5Tojson("'// not a comment'")).assertEqual("'// not a comment'");
    });

    it('preserves escaped quote inside string', Level.LEVEL1, () => {
      expect(json5Tojson('"test\\"end"')).assertEqual('"test\\"end"');
    });
  });

  describe('Json5ToJson.passthrough', () => {
    it('returns empty string for empty input', Level.LEVEL0, () => {
      expect(json5Tojson('')).assertEqual('');
    });

    it('returns unchanged string with no comments', Level.LEVEL0, () => {
      expect(json5Tojson('no comments here')).assertEqual('no comments here');
    });

    it('returns unchanged JSON string', Level.LEVEL1, () => {
      expect(json5Tojson('{"key":"value"}')).assertEqual('{"key":"value"}');
    });
  });

  describe('Json5ToJson.mixed', () => {
    it('removes block comment between JSON fields', Level.LEVEL1, () => {
      expect(json5Tojson('{"a":1 /* c */, "b":2}')).assertEqual('{"a":1 , "b":2}');
    });

    it('preserves URL with double-slash inside string', Level.LEVEL1, () => {
      expect(json5Tojson('"http://example.com"')).assertEqual('"http://example.com"');
    });

    it('removes line comment inside JSON but preserves string content', Level.LEVEL1, () => {
      expect(json5Tojson('{// c\n"a":"// x"}')).assertEqual('{\n"a":"// x"}');
    });
  });

  describe('Json5ToJson.edgeCases', () => {
    it('preserves single slash that is not a comment', Level.LEVEL1, () => {
      expect(json5Tojson('a/b')).assertEqual('a/b');
    });

    it('removes unclosed block comment with only opening marker', Level.LEVEL1, () => {
      expect(json5Tojson('/*')).assertEqual('');
    });

    it('removes nested-looking block comment as one comment', Level.LEVEL1, () => {
      expect(json5Tojson('/*/**/')).assertEqual('');
    });

    it('preserves star-slash inside string', Level.LEVEL1, () => {
      expect(json5Tojson('hello "*/" world')).assertEqual('hello "*/" world');
    });
  });
}
