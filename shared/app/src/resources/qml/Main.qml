import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtQuick.Window

ApplicationWindow {
    id: root
    width: 880
    height: 580
    minimumWidth: 760
    minimumHeight: 480
    visible: true
    title: "HackPass"

    Material.theme: Material.Dark
    Material.accent: Theme.accent
    Material.primary: Theme.primary
    Material.background: Theme.background
    Material.foreground: Theme.foreground
    color: Theme.background

    property string activePage: "login"

    StackView {
        id: stack
        anchors.fill: parent
        initialItem: splashComponent
        replaceEnter: Transition { NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 220 } }
        replaceExit:  Transition { NumberAnimation { property: "opacity"; from: 1; to: 0; duration: 180 } }
    }

    function showSplash()  { stack.replace(splashComponent);   activePage = "splash" }
    function showLogin()   { stack.replace(loginComponent);    activePage = "login" }
    function showVault()   { stack.replace(vaultComponent);    activePage = "vault" }
    function showSettings(){ stack.replace(settingsComponent); activePage = "settings" }
    function showWizard()  { stack.replace(wizardComponent);   activePage = "wizard" }

    Component { id: splashComponent;   SplashScreen   { onFinished: root.showLogin() } }
    Component { id: loginComponent;    LoginPage      { onUnlocked: root.showVault();  onWizardRequested: root.showWizard() } }
    Component { id: vaultComponent;    VaultPage      { onSettingsRequested: root.showSettings(); onLocked: root.showLogin() } }
    Component { id: settingsComponent; SettingsPage   { onBack: root.showVault(); onCreateNewVaultRequested: root.showWizard() } }
    Component { id: wizardComponent;   FirstRunWizard { onDone: root.showVault();  onCancelled: root.showLogin() } }

    ServerMessageDialog  { id: serverMessageDialog }
    TamperDetectedDialog { id: tamperDialog }

    // Watch vault state. If it transitions out of Unlocked (auto-lock fired,
    // manual lock, tamper detection, etc.) while the user is on a page that
    // assumes Unlocked content, force-navigate to the login screen so the
    // VaultPage doesn't render against an empty model and look like the
    // entries disappeared.
    Connections {
        target: typeof vaultManager !== "undefined" ? vaultManager : null
        function onStateChanged(newState, oldState) {
            // 2 = Unlocked, 4 = Syncing
            if (newState !== 2 && newState !== 4) {
                if (root.activePage === "vault" || root.activePage === "settings") {
                    root.showLogin()
                }
            } else if (newState === 2 && root.activePage === "login") {
                root.showVault()
            }
        }
    }
}
