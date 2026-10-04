#pragma once


#include "ARUI/Runtime/IPainter.hpp"
namespace ARUI::Runtime
{

    class AruiFlatPainter : public IPainter
    {

    public:
      void
      Paint(const PaintTreeContext &context,
            const Language::PainterStyleProperties &properties) const override {

      }

      [[nodiscard("")]] const Language::PainterStyleSchema &
      StyleSchema() const noexcept override {
   
      }

      
    };
};