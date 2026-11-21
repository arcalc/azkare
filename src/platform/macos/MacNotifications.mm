/**
 * MacNotifications.mm
 * Objective-C++ bridge: sends a local notification via
 * UNUserNotificationCenter (macOS 10.14+).
 *
 * Called from NotificationDispatcher::sendMacOS().
 */

#import <Foundation/Foundation.h>
#import <UserNotifications/UserNotifications.h>

#ifdef __cplusplus
extern "C" {
#endif

void requestMacOSNotificationPermission(void)
{
    UNUserNotificationCenter *center =
        [UNUserNotificationCenter currentNotificationCenter];
    [center requestAuthorizationWithOptions:
        (UNAuthorizationOptionAlert | UNAuthorizationOptionSound)
        completionHandler:^(BOOL granted, NSError *error) {
            if (!granted)
                NSLog(@"[Azkare] Notification permission not granted: %@", error);
        }];
}

void sendMacOSNotification(const char *title, const char *body)
{
    UNMutableNotificationContent *content =
        [[UNMutableNotificationContent alloc] init];
    content.title = [NSString stringWithUTF8String:title ? title : ""];
    content.body  = [NSString stringWithUTF8String:body  ? body  : ""];
    content.sound = [UNNotificationSound defaultSound];

    // Trigger immediately
    UNTimeIntervalNotificationTrigger *trigger =
        [UNTimeIntervalNotificationTrigger triggerWithTimeInterval:0.1
                                                           repeats:NO];

    NSString *identifier =
        [NSString stringWithFormat:@"azkare-%f", [[NSDate date] timeIntervalSince1970]];

    UNNotificationRequest *request =
        [UNNotificationRequest requestWithIdentifier:identifier
                                             content:content
                                             trigger:trigger];

    [[UNUserNotificationCenter currentNotificationCenter]
        addNotificationRequest:request
        withCompletionHandler:^(NSError *error) {
            if (error)
                NSLog(@"[Azkare] Notification error: %@", error);
        }];
}

#ifdef __cplusplus
}
#endif
