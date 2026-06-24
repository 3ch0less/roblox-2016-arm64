// Stub for missing iOS/SharedCode/ObjectiveCUtilities.h
#pragma once
#import <Foundation/Foundation.h>
#include <string>
#include <boost/function.hpp>

inline std::string stdStringFromNSString(NSString *s) {
    return s ? std::string([s UTF8String]) : std::string();
}
inline NSString* nsStringFromStdString(const std::string& s) {
    return [NSString stringWithUTF8String:s.c_str()];
}

// Wraps an ObjC selector taking one NSString arg as a boost::function<void(std::string)>
template<typename Arg1>
struct BoostFuncFromSelector1Impl {
    SEL selector;
    id target;
    void operator()(const Arg1& arg) const {
        NSString* nsArg = [NSString stringWithUTF8String:arg.c_str()];
        [target performSelectorOnMainThread:selector withObject:nsArg waitUntilDone:NO];
    }
};

template<typename Arg1>
boost::function<void(Arg1)> boostFuncFromSelector_1(SEL selector, id target) {
    BoostFuncFromSelector1Impl<Arg1> impl;
    impl.selector = selector;
    impl.target = target;
    return impl;
}
