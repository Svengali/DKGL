//
//  File: DKFrame.h
//  Author: Hongtae Kim (tiff2766@gmail.com)
//
//  Copyright (c) 2004-2020 Hongtae Kim. All rights reserved.
//

#pragma once
#include "../DKFoundation.h"
#include "DKMatrix3.h"
#include "DKPoint.h"
#include "DKSize.h"
#include "DKRect.h"
#include "DKColor.h"
#include "DKWindow.h"
#include "DKBlendState.h"
#include "DKPixelFormat.h"

namespace DKFramework
{
    class DKScreen;
    class DKCanvas;
    class DKTexture;

    /// @brief
    /// a frame class, used by screen for drawing behaviors.
    /// @details
    /// frame objects can be tree-structured as children and parent. But one screen
    /// can have one root-frame only.
    /// You need subclass of this, and override event handlers and drawing functions.
    ///
    /// A frame can capture keyboard or mouse exclusively, then no other frames will
    /// not receive event. some events require to be captured exclusively.
    ///
    /// Every input events (keyboard, mouse) processed by screen before propagates to
    /// sub frames. All events can be intercepted by frame's ancestors.
    ///
    /// A frame can have its own coordinates space as local content transform, it
    /// does not need to be pixel space and you can have desired aspect ratio.
    ///
    /// default coordinates origin(0,0) is top-left.
    ///
    /// You can use frame as off-screen, then frame does not need to be structured
    /// hierarchically. (off-screen frame can not receive non-captured events.)
    /// Using DKRenderTarget or DKRenderer instead of off-screen frame, is
    /// recommended if you don't need frame's features. (on/off screen with events)
    ///
    /// @note
    ///    frame hierarchical update order is, parent -> child.
    ///    - parent can pass values to child
    ///    frame hierarchical drawing order is, childen -> parent.
    ///    - parent can use children surface(textures) on render.
    ///
    ///    frame must be loaded by calling Load() with screen object before use.
    class DKGL_API DKFrame
    {
    public:
        DKFrame(void);
        virtual ~DKFrame(void);

        /// Load frame and it's children with specified screen object and
        /// desired resolution. Load frame does not affect hierarchy.
        /// Frame should be loaded before use, whether it is off-screen or not.
        void Load(DKScreen* screen, const DKSize& resolution);
        /// Unload frame and it's children.
        /// If frame is off-screen, this function should be called before being
        /// destroyed. (on-screen frame unloads automatically, but you can
        /// unload manually.) Unload frame does not affect hierarchy.
        void Unload(void);
        bool IsLoaded(void) const { return loaded; }

        /// get screen object.
        DKScreen* Screen(void) { return screen; }
        const DKScreen* Screen(void) const { return screen; }

        /// Managing the frame hierarchy
        bool AddSubframe(DKFrame* frame);    ///< add child frame
        void RemoveSubframe(DKFrame* frame); ///< remove child frame
        void RemoveFromSuperframe(void);     ///< remove self from parent

        bool BringSubframeToFront(DKFrame*); ///< bring child to front.
        bool SendSubframeToBack(DKFrame*);   ///< send child to back.

        /// get parent
        DKFrame* Superframe(void) { return superframe; }
        const DKFrame* Superframe(void) const { return superframe; }
        DKFrame* SubframeAtIndex(uint32_t index);
        const DKFrame* SubframeAtIndex(uint32_t index) const;
        bool IsDescendantOf(const DKFrame* frame) const;
        size_t NumberOfSubframes(void) const;
        size_t NumberOfDescendants(void) const;

        /// Frame normalized transform (on parent space, not for content)
        /// set frame's transform (on parent space)
        void SetTransform(const DKMatrix3& transform); 
        const DKMatrix3& Transform(void) const { return transform; }
        const DKMatrix3& TransformInverse(void) const { return transformInverse; }

        /// conversion with local-transform, a local transform have scale, content offset.
        /// transform for local to root coords
        DKMatrix3 LocalFromRootTransform(void) const;  
        /// transform for root to local coords
        DKMatrix3 LocalToRootTransform(void) const;    
        /// transform for local to parent coords
        DKMatrix3 LocalFromSuperTransform(void) const; 
        /// transform for parent to local coords
        DKMatrix3 LocalToSuperTransform(void) const;   

        /// Frame content resize. (in pixel unit)
        /// ContentResolution: returns desired resolution, you can override this.
        ///  default behavior is calculate pixel resolution base on visible region.
        ///  Note: This function will no be called if target frame is root-frame.
        ///        Root frame's resolution will be decided by screen.
        virtual DKSize ContentResolution(void) const;
        /// Set frame to update resolution manually.
        void UpdateContentResolution();

        /// Frame hit-test filter.
        /// You can override this function to use custom shape (non-rectangle).
        /// Return false if pos not inside of frame, a position depended events
        /// will pass to it's parent. (default is true)
        virtual bool HitTest(const DKPoint& pos) const { return true; }

        /// Subframes (children) hit-test filter.
        /// You can override this function to prevent propagation of events to
        /// it's children. If overridden function return false, frame will not
        /// propagates events to it's children. (default is true)
        virtual bool ContentHitTest(const DKPoint& pos) const { return true; }

        /// content scale.
        DKSize ContentScale(void) const;
        void SetContentScale(const DKSize& s);

        /// Bounds returns content bounds (0, 0, width, height)
        DKRect Bounds(void) const;
        /// DisplayBounds returns displaying content bounds.
        /// displaying content bounds can be different from Bounds.
        /// (depends on content-transform)
        /// if frame rotated not to be aligned to axis, display-bounds
        /// could be large enough to contains all edges.
        DKRect DisplayBounds(void) const;

        /// content transform. (not for frame itself)
        /// mouse-event will be applied this transform. (position)
        void SetContentTransform(const DKMatrix3& m);
        const DKMatrix3& ContentTransform(void) const;
        const DKMatrix3& ContentTransformInverse(void) const;

        /// convert coordinates to/from parent.
        DKPoint LocalToSuper(const DKPoint& pt) const;
        DKPoint SuperToLocal(const DKPoint& pt) const;

        /// convert coordinates to/from pixel-based coords.
        /// Note: content transform not applied.
        DKPoint LocalToPixel(const DKPoint& pt) const;
        DKPoint PixelToLocal(const DKPoint& pt) const;
        DKSize LocalToPixel(const DKSize& size) const;
        DKSize PixelToLocal(const DKSize& size) const;
        DKRect LocalToPixel(const DKRect& rect) const;
        DKRect PixelToLocal(const DKRect& rect) const;

        /// capture keyboard, mouse exclusively.
        /// Note:
        ///    Event system designed for multiple-keyboards, multiple-mice.
        ///    You need specify deviceId (starting with 0).
        ///    On multi-touch device, mouse device-Id has same meaning of
        ///    touch-id. (in this case, buttonId is 0 always.)
        bool CaptureKeyboard(int deviceId);
        bool CaptureMouse(int deviceId);
        void ReleaseKeyboard(int deviceId);
        void ReleaseMouse(int deviceId);
        void ReleaseAllKeyboardsCapturedBySelf(void);
        void ReleaseAllMiceCapturedBySelf(void);

        // determine whether frame captured specified device or not
        bool IsKeybaordCapturedBySelf(int deviceId) const;
        bool IsMouseCapturedBySelf(int deviceId) const;

        // get mouse position
        DKPoint MousePosition(int deviceId) const;
        bool IsMouseHover(int deviceId) const;

        // Update: update frame. It is called by system automatically.
        //  Don't call directly unless frame is off-screen and not in hierarchy.
        //  this function invokes OnUpdate() which can be overridden.
        void Update(double tickDelta, DKFoundation::DKTimeTick tick, const DKFoundation::DKDateTime& tickDate);
        /// draw frame. It is called by system automatically,
        /// Don't call directly unless frame is off-screen and not in hierarchy.
        /// this function invokes OnRender() which can be overridden.
        void Draw(void) const;
        /// set frame to be drawn.
        /// You need to call this function if you need to draw something.
        void SetRedraw(void) const;
        /// discard surface.
        /// a surface will be re-created if necessary.
        void DiscardSurface(void);
        /// get frame surface texture.
        const DKTexture* Texture(void) const;


        /// frame base color for blending.
        void SetColor(const DKColor& color);
        DKColor Color(void) const { return color; }
        void SetBlendState(const DKBlendState& blend);
        const DKBlendState& BlendState(void) const;

        /// frame visibility.
        bool IsHidden(void) const { return hidden; }
        bool IsEnabled(void) const { return enabled; }
        void SetHidden(bool hidden);
        void SetEnabled(bool enabled);

        bool CanHandleKeyboard(void) const;
        bool CanHandleMouse(void) const;
        bool IsVisibleOnScreen(void) const;

        /// allow mouse event for local frame (does not affect hierarchical propagation)
        void AllowMouseEvent(bool b);
        bool IsMouseEventAllowed() const { return allowMouseEvent; }
        /// allow keyboard event for local frame (does not affect hierarchical propagation)
        void AllowKeyboardEvent(bool b);
        bool IsKeyboardEventAllowed() const { return allowKeyboardEvent; }

        /// depth-buffer format. (see DKRenderTarget.h)
        /// If frame don't need to render 3D objects,
        /// depth-format can be 'DKRenderTarget::DepthFormatNone'.
        void SetDepthFormat(DKPixelFormat fmt);
        DKPixelFormat DepthFormat(void) const;

    protected:
        // frame events
        /// override to draw custom background.
        virtual void OnDraw(DKCanvas&) const;
        /// called after frame layer drawing is completed.
        virtual void OnDrawOverlay(DKCanvas&) const {}

        /// called every frames.
        virtual void OnUpdate(double, DKTimeTick, DKDateTime) {} 
        /// initialize frame, you can add child frame at here.
        virtual void OnLoaded(void) {}
        /// do something clean-up actions
        virtual void OnUnload(void) {} 
        /// resolution has changed.
        virtual void OnContentResized(void) {} 

        // mouse events
        virtual void OnMouseDown(int deviceId, int buttonId, const DKPoint& pos) {}
        virtual void OnMouseUp(int deviceId, int buttonId, const DKPoint& pos) {}
        virtual void OnMouseMove(int deviceId, const DKPoint& pos, const DKVector2& delta) {}
        virtual void OnMouseWheel(int deviceId, const DKPoint& pos, const DKVector2& delta) {}
        virtual void OnMouseHover(int deviceId) {}
        virtual void OnMouseLeave(int deviceId) {}
        virtual void OnMouseLost(int deviceId) {}  ///< mouse lost which captured by self.

        // keyboard, text events
        virtual void OnKeyDown(int deviceId, DKVirtualKey key) {}
        virtual void OnKeyUp(int deviceId, DKVirtualKey key) {}
        virtual void OnTextInput(int deviceId, const DKString& str) {}
        virtual void OnTextInputCandidate(int deviceId, const DKString& str) {}
        virtual void OnKeyboardLost(int deviceId) {}

        /// Pre-process event.
        /// All ancestors pre-process function will be called,
        /// when event before processing.
        /// return true to cancel event.
        virtual bool PreprocessMouseEvent(const DKWindow::MouseEvent&, const DKPoint& pos, const DKVector2& delta) { return false; }
        virtual bool PreprocessKeyboardEvent(const DKWindow::KeyboardEvent&) { return false; }

    private:
        DKMatrix3 transform;        // frame's transform on parent space.
        DKMatrix3 transformInverse; // convert local coords to parent coords.

        DKArray<DKObject<DKFrame>> subframes;
        DKFrame* superframe;
        DKScreen* screen;

        DKSize contentResolution;  // pixel resolution.
        DKSize contentScale;       // content scale in logical unit.
        DKMatrix3 contentTransform;   // content transform.
        DKMatrix3 contentTransformInverse;
        DKColor::RGBA32 color;
        DKBlendState blendState;

        DKObject<DKTexture> renderTarget;
        DKMap<int, bool> mouseHover;  // store mouse hover states.
        DKPixelFormat depthFormat;

        bool loaded : 1;
        bool hidden : 1;
        bool enabled : 1;

        bool allowMouseEvent : 1;       // affects local frame only.
        bool allowKeyboardEvent : 1;    // affects local frame only.

        mutable bool drawSurface : 1;

        bool DrawInternal(void); // return true, if drawn actually happen.
        bool InsideFrameRect(bool* covered, const DKRect& rect, const DKMatrix3& tm) const; // checking frame covers parent region entirely.
        bool ProcessKeyboardEvent(const DKWindow::KeyboardEvent&);
        bool ProcessMouseEvent(const DKWindow::MouseEvent&, const DKPoint&, const DKVector2&, bool propagate);
        bool ProcessMouseInOut(int deviceId, const DKPoint& pos, bool insideParent);
        void ReleaseMouseData(void);

        friend class DKScreen;
    };
}
