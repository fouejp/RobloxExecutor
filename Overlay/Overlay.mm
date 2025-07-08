
#import "Overlay.h"
#import <UIKit/UIKit.h>

@implementation Overlay

- (id)init {
    self = [super init];
    if (self) {
        self.view = [[UIView alloc] initWithFrame:CGRectMake(30, 100, 300, 200)];
        self.view.backgroundColor = [[UIColor blackColor] colorWithAlphaComponent:0.8];
        self.view.layer.cornerRadius = 12;

        UILabel *title = [[UILabel alloc] initWithFrame:CGRectMake(0, 10, 300, 30)];
        title.text = @"Roblox Executor";
        title.textAlignment = NSTextAlignmentCenter;
        title.textColor = [UIColor whiteColor];
        [self.view addSubview:title];

        scriptField = [[UITextField alloc] initWithFrame:CGRectMake(10, 50, 280, 40)];
        scriptField.placeholder = @"Enter script here";
        scriptField.backgroundColor = [UIColor whiteColor];
        scriptField.textColor = [UIColor blackColor];
        scriptField.layer.cornerRadius = 6;
        [self.view addSubview:scriptField];

        UIButton *runButton = [UIButton buttonWithType:UIButtonTypeSystem];
        runButton.frame = CGRectMake(10, 110, 280, 40);
        [runButton setTitle:@"Run" forState:UIControlStateNormal];
        [runButton setTitleColor:[UIColor whiteColor] forState:UIControlStateNormal];
        runButton.backgroundColor = [UIColor darkGrayColor];
        runButton.layer.cornerRadius = 6;
        [runButton addTarget:self action:@selector(sendToRobloxRemote) forControlEvents:UIControlEventTouchUpInside];
        [self.view addSubview:runButton];

        [[UIApplication sharedApplication].keyWindow addSubview:self.view];
    }
    return self;
}

- (void)sendToRobloxRemote {
    NSString *script = scriptField.text;
    if (script.length == 0) return;

    NSLog(@"[RobloxExecutor] Script: %@", script);

    // Simulated Roblox script execution
}

@end

__attribute__((constructor))
static void initialize() {
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(2 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
        Overlay *overlay = [[Overlay alloc] init];
    });
}
