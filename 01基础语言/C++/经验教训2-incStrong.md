```
system/core/libutils/include/utils/StrongPointer.h:253:16: error: no member named 'incStrong' in 'vendor::hsae::hardware::custom_setting::implementation::CanUtil'
        other->incStrong(this);
        ~~~~~  ^
vendor/hsae/hardware/interfaces/CustomSetting/1.0/default/SerialCallback.cpp:10:14: note: in instantiation of member function 'android::sp<vendor::hsae::hardware::custom_setting::implementation::CanUtil>::operator=' requested here
    mCanUtil = haseFace->mCanUtil;
             ^
```

CanUtil没有incStrong方法。原因是使用android里的智能指针sp，需要集成`android::RefBase`,如下：

```C++
class CanUtil : virtual public ::android::RefBase{
}
```

