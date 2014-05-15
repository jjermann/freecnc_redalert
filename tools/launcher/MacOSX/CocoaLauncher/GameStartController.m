#import "GameStartController.h"
#include "HelperFunctions.h";

#define false 0
#define true 1

@implementation GameStartController
bool fullscreen=false;
bool nosound=false;
NSString *RunLine;
- (IBAction)StartGameEvent:(id)sender
{
    RunLine=@"../../freecnc";
    NSLog(@"fullscreen==%d,   nosound==%d\n",fullscreen,nosound);
    if(fullscreen==true){
        RunLine=[NSString stringWithFormat:@"%@ -fullscreen",RunLine];
    }
    if(nosound==true){
        RunLine=[NSString stringWithFormat:@"%@ -nosound",RunLine];
    }
    //what mission
    RunLine=[NSString stringWithFormat:@"%@ -map %@",RunLine,[Mission stringValue]];
    //what resolution
    //0 is 640x480, 1 is 800x600, 2 is 1024,768
    if([Resolution selectedRow]==0){
                RunLine=[NSString stringWithFormat:@"%@ -w 640 -h 480",RunLine];
    }
    if([Resolution selectedRow]==1){
                RunLine=[NSString stringWithFormat:@"%@ -w 800 -h 600",RunLine];
    }
    if([Resolution selectedRow]==2){
                RunLine=[NSString stringWithFormat:@"%@ -w 1024 -h 768",RunLine];
    }
    //what depth
    //0 is 8, 1 is 16, 2 is 24, 3 is 32
    if([Depth selectedRow]==0){
        RunLine=[NSString stringWithFormat:@"%@ -bpp 8",RunLine];
    }
    if([Depth selectedRow]==1){
RunLine=[NSString stringWithFormat:@"%@ -bpp 16",RunLine];
    }
    if([Depth selectedRow]==2){
RunLine=[NSString stringWithFormat:@"%@ -bpp 24",RunLine];
    }
    if([Depth selectedRow]==3){
RunLine=[NSString stringWithFormat:@"%@ -bpp 32",RunLine];
    }

        system([RunLine cString]);
    NSLog(@"%@\n",RunLine);

}

- (IBAction)SwitchSound:(id)sender{
    nosound=flip(nosound);
}

- (IBAction)SwitchFullScreen:(id)sender
{
    fullscreen=flip(fullscreen);
}


@end