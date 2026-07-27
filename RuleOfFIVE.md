# C++ Resource Ownership & The Rule of Five

A technical reference for wrapping C-style APIs (like `libgphoto2`), managing memory lifetimes with `std::unique_ptr`, and understanding C++ move semantics.

---

## 1. The Code: A Safe C-API Wrapper

When wrapping a C-API, you often need a custom destructor to safely release hardware or network connections before freeing memory. Below is the standard architecture for doing this safely without triggering compiler errors.

```cpp
#include <gphoto2/gphoto2-camera.h>
#include <memory>
#include <print>

// 1. Custom Deleters for C-style pointers
struct ContextDeleter {
    void operator()(GPContext* ctx) const { if (ctx) gp_context_unref(ctx); }
};
struct CameraDeleter {
    void operator()(Camera* cam) const { if (cam) gp_camera_free(cam); }
};

class CameraClient {
private:
    std::unique_ptr<GPContext, ContextDeleter> m_context;
    std::unique_ptr<Camera, CameraDeleter> m_camera;
    bool m_connected = false;

public:
    // Constructor
    CameraClient(CameraAbilities& abilities, GPPortInfo& portInfo) 
        : m_context(gp_context_new()) 
    {
        Camera* raw_cam = nullptr;
        gp_camera_new(&raw_cam);
        gp_camera_set_abilities(raw_cam, abilities);
        gp_camera_set_port_info(raw_cam, portInfo);
        m_camera.reset(raw_cam);
    }

    // Custom Destructor
    ~CameraClient() {
        if (m_camera && m_context) {
            gp_camera_exit(m_camera.get(), m_context.get());
        }
    }

    // ==========================================
    // THE RULE OF FIVE
    // ==========================================
    
    // 1. Explicitly enable Move semantics (Required because we wrote a custom destructor)
    CameraClient(CameraClient&&) noexcept = default;
    CameraClient& operator=(CameraClient&&) noexcept = default;

    // 2. Explicitly delete Copy semantics (Prevents double-free memory corruption)
    CameraClient(const CameraClient&) = delete;
    CameraClient& operator=(const CameraClient&) = delete;

    // ==========================================

    bool connect() {
        if (m_connected) return true;

        std::println("Connection Attempt ... ");
        int ret = gp_camera_init(m_camera.get(), m_context.get());
        if (ret < GP_OK) {
            std::println("Connection Error: {}", ret);
            return false;
        }
        
        std::println("Connected Successfully!");
        m_connected = true;
        return true;
    }
};
```

## 2. The Rule of Five

Every C++ class has five special member functions that manage its lifecycle. If you do not write them, the compiler attempts to silently generate them for you.

| Function | Signature | What it does by default |
|---|---|---|
| **Destructor** | `~T()` | Destroys members in reverse order of creation. |
| **Copy Constructor** | `T(const T&)` | Performs a member-wise copy. |
| **Copy Assignment** | `operator=(const T&)` | Performs a member-wise copy assignment. |
| **Move Constructor** | `T(T&&)` | Performs a member-wise move. |
| **Move Assignment** | `operator=(T&&)` | Performs a member-wise move assignment. |

**The Golden Rule:** If you manually define *any* of these five functions, you should explicitly define (or `= delete`, or `= default`) all of them.

If a class has a user-declared destructor, C++ strictly disables the implicit generation of the Move Constructor and Move Assignment operator.

---

## 3. The Mechanics of Copy vs. Move

### Copy Semantics (Lvalues)
An **lvalue** (locator value) is an object that occupies a persistent memory location. It has a name. When you copy an lvalue, the compiler allocates entirely new memory and duplicates the bits. For pointers, a default copy is a "shallow copy"—it copies the memory address, which leads to double-free crashes if both objects try to free the same address.

### Move Semantics (Rvalues)
An **rvalue** is a temporary object (e.g., data that is about to be destroyed). Because an rvalue is dying anyway, C++ allows you to **steal** its resources. Instead of allocating new memory, a Move Constructor (`T&&`) copies the pointer addresses from the source, and then sets the source's pointers to `nullptr`. 

### The Truth About `std::move`
`std::move` does not actually move anything. At the compiler level, `std::move(myObj)` is just a cast: `static_cast<T&&>(myObj)`. It simply tells the compiler: *"Treat this named lvalue as if it were a temporary rvalue."* The actual moving of data is done entirely by the Move Constructor.

---

## 4. Autopsy of a Compiler Error

**Error:** `call to implicitly-deleted copy constructor`  
**Trigger:** `myVector.push_back(std::move(myObject));`

Here is exactly how the C++ compiler processes this failure:

1. **You wrote a custom destructor.**
   * *Compiler thought:* "They are managing resources manually. I will NOT auto-generate a Move Constructor."
2. **You called `std::move(myObject)`.**
   * *Compiler thought:* "They cast the object to an rvalue reference (`T&&`). I need to find a constructor that accepts an rvalue."
3. **The compiler looked for a Move Constructor.**
   * *Compiler thought:* "There is no `T(T&&)`. What is my fallback?"
4. **The compiler fell back to the Copy Constructor.**
   * In C++, an rvalue can legally bind to a `const` lvalue reference. So, the compiler attempts to use the Copy Constructor (`T(const T&)`).
5. **The compiler checked member variables.**
   * To build the default Copy Constructor, it must copy every member variable. It looks at `std::unique_ptr`.
6. **The fatal flaw.**
   * `std::unique_ptr` explicitly `= delete`s its own copy constructor (unique pointers cannot be shared). 
   * *Compiler thought:* "I cannot copy the `unique_ptr`. Therefore, I cannot generate a copy constructor for the class. I give up." -> **FATAL ERROR**.

**The Fix:** Explicitly adding `T(T&&) noexcept = default;` allows the compiler to find the move constructor at Step 3, bypass the copy fallback, and cleanly transfer the unique pointers.