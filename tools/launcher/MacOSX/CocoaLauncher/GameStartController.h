/* GameStartController */

#import <Cocoa/Cocoa.h>

@interface GameStartController : NSObject
{
    IBOutlet id Depth;
    IBOutlet id Resolution;
    IBOutlet id Mission;
}
- (IBAction)StartGameEvent:(id)sender;
- (IBAction)SwitchFullScreen:(id)sender;
- (IBAction)SwitchSound:(id)sender;

@end
