#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/rcupdate.h>
#include <linux/hashtable.h>
#include <linux/jiffies.h>
#include "zero_visor.h"
#include "process_trace.h"

#define MAX_CPU 32
#define EXIT_TIMEOUT 5*HZ

static u64 last_cr3[MAX_CPU]={0};

DEFINE_HASHTABLE(pid_ht,12);

//描述pid
struct zv_pid_entry
{
    u32 pid;
    unsigned long last_jiffies;
    struct hlist_node hnode;
};

static DEFINE_HASHTABLE(pid_lock);

//捕捉CR3切换，间接表示进程切换，后续匹配输出信息
void zv_trace_cr3_switch(int cpu , u64 new_cr3){
    new_cr3&=~0xFFFULL;

    if( cpu>MAX_CPU || g_last_cr3(cpu)==new_cr3){
        return;
    }
    last_cr3[cpu]=new_cr3;

    rcu_read_lock();

    {
        u64 pgd_phys;
        struct task_struct *p;
        //白名单
        static const char *ignore_list[] = {
            "rsylogd",
            "systemd-journald",
            "syslog-ng",
            NULL   
        };


        //遍历慢，感觉可以优化。
        for_each_process(p){
            if(!task->mm) continue;

            pgd_phys=virt_to_phys(p->mm->pgd)&(~0xFFFULL);
            
            if(pgd_phys!=new_cr3) continue;
            
            //过滤白名单
            {
                const char **name;
                for(name = ignore_list; *name; ++name){
                    if(strcmp(p->comm, *name)==0){
                        rcu_read_unlock();
                        return;
                    }
                }
            }

            //节流＋去重
            {
                struct zv_pid_entry *e;
                unsigned long flags;

                hash_for_each_possible(pid_ht,e,hnode,p->pid)
                {
                    if(e->pid==p->pid)
                    {
                        if(time_before(jiffies,e->last_jiffies + HZ))
                        {
                            e->last_jiffies = jiffies;
                            rcu_read_unlock();
                            return;
                        }
                        break;
                    }
                }
                if(!e)
                {
                    spin_lock_irqsave(&pid_lock, flags);
                    e=kmalloc(sizeof(*e),GFP_ATOMIC);
                    if(e){
                        e->pid = p->pid;
                        hash_add(pid_ht, &e->hnode, e->pid);
                    }
                    spin_unlock_irqrestore(&pid_lock, flags);
                }

                if(e) {
                    e->last_jiffies = jiffies;
                }
            }

            //匹配的输出信息到日志
            {
                zv_printf(LOG_LEVEL_NORMAL,"[PROCESS_TRACE] ZV_TRACE: CPU=%d pid=%d comm=%s ppid=%d\n",
                    cpu,p->pid,p->comm,p->real_parent->ppid);
                break;
            }
        }
        
    }
    rcu_read_unlock();

}



void zv_exit_scan(struct time_list *t){
    unsigned long now = jiffies;
    struct sb_entry_pid *e;

    spin_lock_irq(&pid_lock);

    hash_for_each_safe(pid_ht, bkt, tmp, e, hnode);
    {
        if(time_after(now,e->jiffies + EXIT_TIMEOUT )){
            //打印日志
            sb_printf(LOG_LEVEL_NORMAL,
                "[PROCESS_TRACE] ZV_TRACE: EXIT CPU=%d pid=%d\n", cpu, p->pid);

            hash_del(&e->hnode);
            kfree(e);
        }
    }
    spin_unlock_irq(&pid_lock);

    mod_timer();

}

void zv_exit_scan_timer_setup(void)
{
    init_timer(&exit_scan_timer);
    exit_scan_timer.function=
}