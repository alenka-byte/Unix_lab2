#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/time.h>
#include <linux/rtc.h>

#define PROC_FILENAME "tsulab"
static struct proc_dir_entry *proc_file = NULL;

static int calculate_time(void) {
    struct timespec64 now;
    struct rtc_time tm;
    ktime_get_real_ts64(&now);
    rtc_time64_to_tm(now.tv_sec, &tm);
    return (23 - tm.tm_hour) * 3600 + (59 - tm.tm_min) * 60 + (59 - tm.tm_sec);
}
static ssize_t procfile_read( struct file *file_pointer, char __user *buffer, size_t buffer_length, loff_t* offset) {
    char result[128];
    int len;
    int time = calculate_time();
    if (time > 0) 
         len = snprintf(result, sizeof(result), "Cinderella has %d  seconds until midnight\n" "It's %d hours, %d minutes, %d seconds\n", time, time / 3600, (time % 3600) / 60, time % 60);
    else
        len = snprintf(result, sizeof(result), "It's midnight!");

    if (*offset >= len)
        return 0;
    if (buffer_length < len - *offset)
        len = len - *offset;

    if (copy_to_user(buffer, result, len))
        return -EFAULT;

    *offset += len;
    pr_info("procfile read %s\n", file_pointer->f_path.dentry->d_name.name);
    return len;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0)
static const struct proc_ops tsulab_fops = { 
    .proc_read = procfile_read,
};
#else
static  const struct file_operations tsulab_fops = {
    .read = procfile_read,
};
#endif
static int __init tsu_module_init(void) 
{
    pr_info("Welcome to the Tomsk State University\n");
    proc_file = proc_create(PROC_FILENAME, 0444, NULL, &tsulab_fops);
    if (!proc_file) {
        pr_err("Failed to create /proc/%s\n", PROC_FILENAME);
        return -ENOMEM;
    }
    pr_info("/proc/%s created\n", PROC_FILENAME);
    return 0;
}

static void __exit tsu_module_exit(void)
{
    if (proc_file) {
        proc_remove(proc_file);
        pr_info("/proc/%s removed\n", PROC_FILENAME);
    }
    pr_info("Tomsk State University forever!\n");
}

module_init(tsu_module_init);
module_exit(tsu_module_exit);
MODULE_LICENSE("GPL");
