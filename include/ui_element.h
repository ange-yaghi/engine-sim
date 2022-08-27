#ifndef ATG_ENGINE_SIM_UI_ELEMENT_H
#define ATG_ENGINE_SIM_UI_ELEMENT_H

#include "ui_math.h"

#include "delta.h"

#include <vector>

class EngineSimApplication;
class UiElement {
    public:
        enum class Event {
            Clicked
        };

    public:
        UiElement();
        virtual ~UiElement();

        virtual void initialize(EngineSimApplication *app);
        virtual void destroy();

        virtual void update(float dt);
        virtual void render();

        virtual void signal(UiElement *element, Event event);
        virtual void onMouseDown(const Point &mouseLocal);
        virtual void onMouseUp(const Point &mouseLocal);
        virtual void onMouseClick(const Point &mouseLocal);
        virtual void onDrag(const Point &p0, const Point &mouse0, const Point &mouse);
        virtual void onMouseOver(const Point &mouseLocal);
        virtual void onMouseLeave();
        virtual void onMouseScroll(int mouseScroll);

        bool isMouseOver() const { return m_mouseOver; }
        bool isMouseHeld() const { return m_mouseHeld; }

        template <typename T_Element>
        T_Element *addElement(UiElement *signalTarget = nullptr) {
            T_Element *newElement = new T_Element;
            newElement->initialize(m_app);
            newElement->m_parent = this;
            newElement->m_signalTarget = signalTarget;
            newElement->m_index = (int)m_children.size();
            m_children.push_back(newElement);

            return newElement;
        }

        UiElement *mouseOver(const Point &mouseLocal);

        Point getWorldPosition() const;

        Point getLocalPosition() const { return m_localPosition; }
        void setLocalPosition(const Point &p) { m_localPosition = p; }
        void setLocalPosition(const Point &p, const Point &ref);

        Point worldToLocal(const Point &wp) const { return wp - getWorldPosition(); }
        Point localToWorld(const Point &lp) const { return lp + getWorldPosition(); }

        void setVisible(bool visible) { m_visible = visible; }
        bool isVisible() const { return m_visible; }

        size_t getChildCount() const { return m_children.size(); }

        void bringToFront(UiElement *element);
        void activate();

        Bounds m_bounds;

    protected:
        void signal(Event event);

        float pixelsToUnits(float length) const;
        Point pixelsToUnits(const Point &p) const;
        float unitsToPixels(float x) const;
        Point unitsToPixels(const Point &p) const;
        Point getRenderPoint(const Point &p) const;
        Bounds getRenderBounds(const Bounds &b) const;
        Bounds unitsToPixels(const Bounds &b) const;

        void resetShader();

        void drawModel(
                dbasic::ModelAsset *model,
                const ysVector &color,
                const Point &p,
                const Point &scale = { 1.0f, 1.0f });
        void drawFrame(
                const Bounds &bounds,
                float thickness,
                const ysVector &frameColor,
                const ysVector &fillColor,
                bool fill = true);
        void drawBox(
                const Bounds &bounds,
                const ysVector &fillColor);
        void drawText(
                const std::string &s,
                const Bounds &bounds,
                float height,
                const Point &ref = Bounds::tl);
        void drawAlignedText(
                const std::string &s,
                const Bounds &bounds,
                float height,
                const Point &ref = Bounds::tl,
                const Point &refText = Bounds::tl);
        void drawCenteredText(
                const std::string &s,
                const Bounds &bounds,
                float height,
                const Point &ref = Bounds::tm);

    protected:
        std::vector<UiElement *> m_children;
        UiElement *m_parent;
        UiElement *m_signalTarget;

    protected:
        Point m_localPosition;
        Bounds m_mouseBounds;
        bool m_checkMouse;
        bool m_disabled;
        int m_index;

        bool m_draggable;
        bool m_mouseOver;
        bool m_mouseHeld;
        bool m_visible;

    protected:
        EngineSimApplication *m_app;
};

#endif /* ATG_ENGINE_SIM_UI_ELEMENT_H */
