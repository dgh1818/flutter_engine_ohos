import smokeTest from './smoke.test';
import toolUtilsTest from './util/ToolUtils.test';

export default function testsuite() {
  smokeTest();
  toolUtilsTest();
}
