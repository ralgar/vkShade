project('VulkanMemoryAllocator', ['c', 'cpp'], version: '3.1.0')

vma_allocator_dep = declare_dependency(
    include_directories: include_directories('include', is_system: true)
)

if get_option('build_samples')
    subdir('src')
endif
