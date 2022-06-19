Base.@kwdef struct Platform
    arch::String
    rootfs_tag::String
    rootfs_treehash::String
    allow_fail::Bool
    commit_status::Bool
end

struct Platforms
    ps::Vector{Platform}
end

Base.length(platforms::Platforms)         = Base.length(platforms.ps)
Base.iterate(platforms::Platforms)        = Base.iterate(platforms.ps)
Base.iterate(platforms::Platforms, state) = Base.iterate(platforms.ps, state)

const platforms = Platforms(
    [
        Platform(;
            arch            = "x86_64",
            rootfs_tag      = "v5.22",
            rootfs_treehash = "0",
            allow_fail      = false,
            commit_status   = true,
        ),
        Platform(;
            arch            = "aarch64",
            rootfs_tag      = "v5.22",
            rootfs_treehash = "0",
            allow_fail      = true,
            commit_status   = false,
        ),
    ]
)
