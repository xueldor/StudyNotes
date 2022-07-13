当RecyclerView发生添加、删除一项时，过去经常使用`notifyDataSetChanged()`一刷到底，但是这样会导致整个RecyclerView重绘，数据较多时影响性能。RecyclerView 提供了更新局部的方法：

`notifyItemChanged、notifyItemInserted、notifyItemMoved、notifyItemRemoved、notifyItemRangeChanged。`

但是notifyItemInserted(int)、notifyItemMoved(int)、notifyItemRemoved(int)  **有坑**记得要填，如下：

RecyclerView的适配器RecyclerView.Adapter需要实现`void onBindViewHolder(ViewHolder viewHolder,  int i)`,我们经常在里面实现点击事件：

```java
@Override
public void onBindViewHolder(ViewHolder viewHolder, final int position) {
    viewHolder.mBtnDelete.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (onDeleteClickListener != null){
                    onDeleteClickListener.onClick(position);
                }
            }
        });
}
```

第二个参数position表示当前item的索引，如果数据修改后，调用的`notifyDataSetChanged`,那么这个方法会重新执行，position自然是修改后的索引。但是如果删除了一条数据，然后调用notifyItemRemoved，那么这个方法不会重新执行，position还是原来的。

比方说，recyclerView有{a,b,c,d,e,f} 6项，第一次点击删除a，没问题。删除后，就变成了{b,c,d,e,f}, b的索引变成了0，但是因为onBindViewHolder没有重新执行，索引OnClickListener里的position仍然是1，于是第二次点击删除b时，实际删除的是c。

**对策：**

方法一：在notifyItemRemoved后面加一行notifyItemRangeChanged

```java
data.remove(position);
mAdapter.notifyItemRemoved(position);
mAdapter.notifyItemRangeChanged(position, data.size() - position);
```

效果是从position开始到最后一项，会刷新，于是每一项都能获得最新的position。但是如果删除的是第一项，那么和`notifyDataSetChanged`性能上好像就没什么区别了。

notifyItemInserted、notifyItemMoved同理。

方法二：将`onDeleteClickListener.onClick(position)`改成`onDeleteClickListener.onClick(viewHolder.getLayoutPosition());`

getLayoutPosition是能获取正确的position的,但是该方法可能返回-1(比如连续快速点击删除)，需要特别处理下。

相关方法有三个：

```java
System.out.println("xxxx 1 " + viewHolder.getAbsoluteAdapterPosition());
System.out.println("xxxx 2 " + viewHolder.getLayoutPosition());
System.out.println("xxxx 4 " + viewHolder.getBindingAdapterPosition());
```

> getLayoutPosition   返回布局中最新的计算位置，和用户所见到的位置一致，当做用户输入（例如点击事件）的时候考虑使用
>
> getBindingAdapterPosition getAbsoluteAdapterPosition    返回数据在Adapter中的位置（也许位置的变化还未来得及刷新到布局中），当使用Adapter的时候（例如调用Adapter的notify相关方法时）考虑使用
>
> getBindingAdapterPosition getAbsoluteAdapterPosition是原来的getAdapterPosition()方法废弃后分化出来的新方法。存在多个adapter嵌套时，getAdapterPosition会存在歧义，所以废弃，用getBindingAdapterPosition 获取元素位于当前绑定Adapter的位置。用getAbsoluteAdapterPosition获取元素位于Adapter中的绝对位置。
>
> 所谓的"多个adapter嵌套时",可以参考https://cloud.tencent.com/developer/article/1620018。MergeAdapter已经更名为“ConcatAdapter”。

小结：

1. onBindViewHolder参数传进来的position仅方法局部内使用，不要保存到别处、不要标志成final传给内部类,因为新增、删除后值会变
2. getLayoutPosition 和 getBindingAdapterPosition 通常情况下是一样的，只有当 Adapter 里面的内容改变了，而 Layout 还没来得及绘制的这段时间之内才有可能不一样，这个时间非常短暂，性能流畅时小于16ms。
3. 如果你没有使用ConcatAdapter，那么getBindingAdapterPosition()和getAbsoluteAdapterPosition()方法的效果是一模一样的。
4. 如果你使用了ConcatAdapter，getBindingAdapterPosition()得到的是元素位于当前绑定Adapter的位置，而getAbsoluteAdapterPosition()方法得到的是元素位于合并后Adapter的绝对位置。



