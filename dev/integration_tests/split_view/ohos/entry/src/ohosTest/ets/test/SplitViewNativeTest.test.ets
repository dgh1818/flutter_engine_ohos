import { describe, expect, it, Level } from '@ohos/hypium';
import { abilityDelegatorRegistry, Driver, ON } from '@kit.TestKit';
import { Want } from '@kit.AbilityKit';
import { hilog } from '@kit.PerformanceAnalysisKit';

const DOMAIN: number = 0x0000;
const TAG: string = '[SplitViewNativeTest]';

const delegator: abilityDelegatorRegistry.AbilityDelegator = abilityDelegatorRegistry.getAbilityDelegator();

const BUNDLE_NAME: string = 'com.example.split_view';
const ENTRY_ABILITY: string = 'EntryAbility';

/**
 * Launches the Flutter app and waits for it to become idle.
 * Each test case creates its own Driver instance inside it(),
 * matching the pattern used by Ability.test.ets.
 */
async function launchApp(driver: Driver): Promise<void> {
  const want: Want = {
    bundleName: BUNDLE_NAME,
    abilityName: ENTRY_ABILITY,
  };
  await delegator.startAbility(want);
  await driver.waitForIdle(3000, 8000);
}

export default function splitViewNativeTest() {
  describe('SplitViewNativeTest', () => {
    // -----------------------------------------------------------------------
    // Test 1: 主页输入框可以正常输入
    //   点击主页原生 TextInput，输入文本，验证文本被正确接受。
    // -----------------------------------------------------------------------
    it('homeInputAcceptsText', Level.LEVEL3, async (done: Function) => {
      const driver = Driver.create();
      await launchApp(driver);
      hilog.info(DOMAIN, TAG, '%{public}s', '=== homeInputAcceptsText START ===');

      // 断言主页原生输入框存在
      hilog.info(DOMAIN, TAG, '%{public}s', '查找主页原生输入框 native_home_text_input...');
      await driver.assertComponentExist(ON.id('native_home_text_input'));
      const input = await driver.findComponent(ON.id('native_home_text_input'));
      hilog.info(DOMAIN, TAG, '%{public}s', '点击输入框...');
      await input.click();
      await driver.waitForIdle(1000, 2000);

      // 输入文本
      hilog.info(DOMAIN, TAG, '%{public}s', '输入文本: hello ohos');
      await input.inputText('hello ohos');
      await driver.waitForIdle(2000, 5000);

      // 读取文本并验证
      const text = await input.getText();
      hilog.info(DOMAIN, TAG, '%{public}s', `读取到的文本: ${text}`);
      expect(text).assertEqual('hello ohos');
      hilog.info(DOMAIN, TAG, '%{public}s', '=== homeInputAcceptsText END ===');

      done();
    });

    // -----------------------------------------------------------------------
    // Test 2: Native页弹窗可以正常触发、退出，输入框可以正常输入
    //   导航到 Native View 页 → 点击原生按钮触发弹窗 → 验证弹窗出现
    //   → 点击 Close 关闭弹窗 → 验证弹窗已关闭 → 在原生输入框输入文本并验证
    // -----------------------------------------------------------------------
    it('nativeViewDialogAndInput', Level.LEVEL3, async (done: Function) => {
      const driver = Driver.create();
      await launchApp(driver);
      hilog.info(DOMAIN, TAG, '%{public}s', '=== nativeViewDialogAndInput START ===');

      // 导航到 Native View 页（按钮在 Navigation 区域第一个，无需滑动）
      hilog.info(DOMAIN, TAG, '%{public}s', '查找 Push Native View Page 按钮...');
      await driver.assertComponentExist(ON.text('Push Native View Page'));
      const pushBtn = await driver.findComponent(ON.text('Push Native View Page'));
      await pushBtn.click();
      await driver.waitForIdle(5000, 10000);
      hilog.info(DOMAIN, TAG, '%{public}s', '已导航到 Native View 页');

      // 点击原生 "Show Native Dialog" 按钮触发弹窗
      hilog.info(DOMAIN, TAG, '%{public}s', '查找 Show Native Dialog 按钮...');
      await driver.assertComponentExist(ON.text('Show Native Dialog'));
      const dialogBtn = await driver.findComponent(ON.text('Show Native Dialog'));
      await dialogBtn.click();
      await driver.delayMs(1000);
      hilog.info(DOMAIN, TAG, '%{public}s', '已点击 Show Native Dialog');

      // 验证弹窗已出现
      hilog.info(DOMAIN, TAG, '%{public}s', '验证弹窗已出现...');
      await driver.assertComponentExist(ON.text('Native Dialog'));
      hilog.info(DOMAIN, TAG, '%{public}s', '弹窗已出现');

      // 点击 "Close" 按钮关闭弹窗
      hilog.info(DOMAIN, TAG, '%{public}s', '查找 Close 按钮...');
      await driver.assertComponentExist(ON.text('Close'));
      const closeBtn = await driver.findComponent(ON.text('Close'));
      await closeBtn.click();
      await driver.delayMs(1000);
      hilog.info(DOMAIN, TAG, '%{public}s', '已点击 Close 关闭弹窗');

      // 验证弹窗已关闭（Native Dialog 文本不再存在）
      // findComponent 在组件不存在时会抛异常，用 try-catch 判断
      let dialogStillOpen = false;
      try {
        const dialogCheck = await driver.findComponent(ON.text('Native Dialog'));
        if (dialogCheck) {
          dialogStillOpen = true;
        }
      } catch (e) {
        // 组件未找到 — 弹窗已关闭
      }
      hilog.info(DOMAIN, TAG, '%{public}s', `弹窗是否仍打开: ${dialogStillOpen}`);
      expect(dialogStillOpen).assertEqual(false);

      // 在 Native View 页原生输入框中输入文本
      hilog.info(DOMAIN, TAG, '%{public}s', '查找 Native View 页原生输入框...');
      await driver.assertComponentExist(ON.id('native_view_text_input'));
      const input = await driver.findComponent(ON.id('native_view_text_input'));
      await input.click();
      await driver.delayMs(500);

      hilog.info(DOMAIN, TAG, '%{public}s', '输入文本: native view');
      await input.inputText('native view');
      await driver.delayMs(500);

      const text = await input.getText();
      hilog.info(DOMAIN, TAG, '%{public}s', `读取到的文本: ${text}`);
      expect(text).assertEqual('native view');

      // 收回软键盘
      hilog.info(DOMAIN, TAG, '%{public}s', '收回软键盘...');
      await driver.pressBack();
      await driver.waitForIdle(1000, 3000);

      // 返回主页，避免影响后续测试的导航状态
      hilog.info(DOMAIN, TAG, '%{public}s', '返回主页...');
      await driver.pressBack();
      await driver.waitForIdle(1000, 3000);
      hilog.info(DOMAIN, TAG, '%{public}s', '=== nativeViewDialogAndInput END ===');

      done();
    });

    // -----------------------------------------------------------------------
    // Test 3: 依次点击四个输入框，验证光标焦点正常跟随
    //   四个输入框：
    //     ① 主页 Dart TextField (home-dart-input)
    //     ② 主页原生 TextInput (native_home_text_input)
    //     ③ Native页 Dart TextField (native-dart-input)
    //     ④ Native页原生 TextInput (native_view_text_input)
    //   操作顺序：先在主页点①②，然后导航到 Native 页点③④
    //
    //   Dart TextField 无法被 ON.text()/ON.id() 直接定位（Flutter 渲染的
    //   组件，hintText 不暴露为 text 属性），因此通过旁边的定位 Text 获取
    //   坐标，再用 driver.click(x, y) 点击 TextField 区域。
    //
    //   断言：每次点击输入框后，验证组件获得焦点（即软键盘已弹出）
    //   - 原生 TextInput 用 Component.isFocused() 检查焦点
    //   - Dart TextField 用 ON.focused(true) 检查焦点（Flutter 语义）
    //   - OHOS 系统在文本输入组件获得焦点时会自动弹出软键盘，
    //     因此焦点 == 软键盘弹出
    // -----------------------------------------------------------------------
    it('cursorFollowsAcrossInputs', Level.LEVEL3, async (done: Function) => {
      const driver = Driver.create();
      await launchApp(driver);
      hilog.info(DOMAIN, TAG, '%{public}s', '=== cursorFollowsAcrossInputs START ===');

      // ① 点击主页 Dart TextField
      //    通过定位 Text 'Home Dart Input Label' 获取坐标，向右偏移点击 TextField
      hilog.info(DOMAIN, TAG, '%{public}s', '[步骤①] 查找 Home Dart Input Label...');
      await driver.assertComponentExist(ON.text('Home Dart Input Label'));
      const homeLabel = await driver.findComponent(ON.text('Home Dart Input Label'));
      const homeLabelBounds = await homeLabel.getBounds();
      const homeDartX = homeLabelBounds.right + 50;
      const homeDartY = Math.floor((homeLabelBounds.top + homeLabelBounds.bottom) / 2);
      hilog.info(DOMAIN, TAG, '%{public}s', `[步骤①] Label bounds: left=${homeLabelBounds.left}, top=${homeLabelBounds.top}, right=${homeLabelBounds.right}, bottom=${homeLabelBounds.bottom}`);
      hilog.info(DOMAIN, TAG, '%{public}s', `[步骤①] 点击坐标: x=${homeDartX}, y=${homeDartY}`);
      await driver.click(homeDartX, homeDartY);
      await driver.delayMs(1000);
      // 断言：有组件获得焦点（Dart TextField 获得焦点）
      hilog.info(DOMAIN, TAG, '%{public}s', '[步骤①] 检查是否有组件获得焦点...');
      let step1Focused = false;
      try {
        await driver.assertComponentExist(ON.focused(true));
        step1Focused = true;
      } catch (e) {
        hilog.info(DOMAIN, TAG, '%{public}s', `[步骤①] 焦点断言失败: ${e}`);
      }
      hilog.info(DOMAIN, TAG, '%{public}s', `[步骤①] focused = ${step1Focused}`);
      // 断言：组件获得焦点（OHOS 文本输入获得焦点即弹出软键盘）
      expect(step1Focused).assertTrue();
      await driver.delayMs(500);

      // ② 点击主页原生 TextInput
      hilog.info(DOMAIN, TAG, '%{public}s', '[步骤②] 查找主页原生 TextInput...');
      await driver.assertComponentExist(ON.id('native_home_text_input'));
      const homeNativeInput = await driver.findComponent(ON.id('native_home_text_input'));
      await homeNativeInput.click();
      await driver.delayMs(1000);
      // 断言：原生输入框获得焦点
      const homeNativeFocused = await homeNativeInput.isFocused();
      hilog.info(DOMAIN, TAG, '%{public}s', `[步骤②] homeNativeInput.isFocused() = ${homeNativeFocused}`);
      // 断言：组件获得焦点（即软键盘已弹出）
      expect(homeNativeFocused).assertTrue();
      await driver.delayMs(500);

      // 导航到 Native View 页（按钮在 Navigation 区域第一个，无需滑动）
      hilog.info(DOMAIN, TAG, '%{public}s', '[导航] 查找 Push Native View Page 按钮...');
      await driver.assertComponentExist(ON.text('Push Native View Page'));
      const pushBtn = await driver.findComponent(ON.text('Push Native View Page'));
      await pushBtn.click();
      await driver.waitForIdle(5000, 10000);
      hilog.info(DOMAIN, TAG, '%{public}s', '[导航] 已导航到 Native View 页');

      // ③ 点击 Native 页 Dart TextField
      //    通过定位 Text 'Native Dart Input Label' 获取坐标，向右偏移点击 TextField
      hilog.info(DOMAIN, TAG, '%{public}s', '[步骤③] 查找 Native Dart Input Label...');
      await driver.assertComponentExist(ON.text('Native Dart Input Label'));
      const nativeLabel = await driver.findComponent(ON.text('Native Dart Input Label'));
      const nativeLabelBounds = await nativeLabel.getBounds();
      const nativeDartX = nativeLabelBounds.right + 50;
      const nativeDartY = Math.floor((nativeLabelBounds.top + nativeLabelBounds.bottom) / 2);
      hilog.info(DOMAIN, TAG, '%{public}s', `[步骤③] Label bounds: left=${nativeLabelBounds.left}, top=${nativeLabelBounds.top}, right=${nativeLabelBounds.right}, bottom=${nativeLabelBounds.bottom}`);
      hilog.info(DOMAIN, TAG, '%{public}s', `[步骤③] 点击坐标: x=${nativeDartX}, y=${nativeDartY}`);
      await driver.click(nativeDartX, nativeDartY);
      await driver.delayMs(1000);
      // 断言：有组件获得焦点（Dart TextField 获得焦点）
      hilog.info(DOMAIN, TAG, '%{public}s', '[步骤③] 检查是否有组件获得焦点...');
      let step3Focused = false;
      try {
        await driver.assertComponentExist(ON.focused(true));
        step3Focused = true;
      } catch (e) {
        hilog.info(DOMAIN, TAG, '%{public}s', `[步骤③] 焦点断言失败: ${e}`);
      }
      hilog.info(DOMAIN, TAG, '%{public}s', `[步骤③] focused = ${step3Focused}`);
      // 断言：组件获得焦点（即软键盘已弹出）
      expect(step3Focused).assertTrue();
      await driver.delayMs(500);

      // ④ 点击 Native 页原生 TextInput
      hilog.info(DOMAIN, TAG, '%{public}s', '[步骤④] 查找 Native 页原生 TextInput...');
      await driver.assertComponentExist(ON.id('native_view_text_input'));
      const nativeViewInput = await driver.findComponent(ON.id('native_view_text_input'));
      await nativeViewInput.click();
      await driver.delayMs(1000);
      // 断言：原生输入框获得焦点
      const nativeViewFocused = await nativeViewInput.isFocused();
      hilog.info(DOMAIN, TAG, '%{public}s', `[步骤④] nativeViewInput.isFocused() = ${nativeViewFocused}`);
      // 断言：组件获得焦点（即软键盘已弹出）
      expect(nativeViewFocused).assertTrue();
      await driver.delayMs(500);
      hilog.info(DOMAIN, TAG, '%{public}s', '=== cursorFollowsAcrossInputs END ===');
      await driver.pressBack();
      await driver.waitForIdle(1000, 3000);

      done();
    });
  });
}
