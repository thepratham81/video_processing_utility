from kivy.properties import NumericProperty,ObjectProperty
from kivy.lang import Builder
from kivymd.uix.boxlayout import MDBoxLayout

kv = '''
<CSpinner>
    size_hint_y:None
    height:dp(root.h)
    MDTextFieldRect:
        id: txt_spinner
        text: str(root.value)
    MDBoxLayout:
        size_hint_x:0.2
        orientation:"vertical"
        Button:
            text:"+"
            size_hint_y:0.5
            on_release: root.increment()
        Button:
            text:"-"
            size_hint_y:0.5
            on_release: root.decrement()
'''

Builder.load_string(kv)

class CSpinner(MDBoxLayout):
    prev_x = NumericProperty(0)
    value  = NumericProperty(0)
    min    = NumericProperty(0)
    max    = NumericProperty(100)
    h      = NumericProperty(32)
    def __init__(self,*args,**kwargs):
        super(CSpinner, self).__init__(*args,**kwargs)
        self.register_event_type('on_change')

    def on_change(self):
        pass
        

    def increment(self):
        if self.value + 1 <= self.max: 
            self.value += 1
        self.dispatch('on_change')

    def decrement(self):
        if self.value - 1 >= self.min:
            self.value -= 1
        self.dispatch('on_change')

    def on_touch_down(self, touch):
        super().on_touch_down(touch)
        if self.ids.txt_spinner.collide_point(*touch.pos):
            self.prev_x = touch.x
            touch.grab(self)
            return True
        return False

    def on_touch_move(self, touch):
        super().on_touch_move(touch)
        if touch.grab_current == self:
            dx = touch.x - self.prev_x
            if dx != 0:
                if abs(dx) < 3:
                    return True
                if dx > 0:
                    self.increment()
                else:
                    self.decrement()
            self.prev_x = touch.x
            return True
        return False

    def on_touch_up(self, touch):
        super().on_touch_up(touch)
        if touch.grab_current == self.ids.txt_spinner:
            touch.ungrab(self.ids.txt_spinner)
            return True
        return False


if __name__ == "__main__":
    from kivymd.app import MDApp
    class SpinnerApp(MDApp):
        def build(self):
            layout = MDBoxLayout()
            cspinner = CSpinner()
            layout.add_widget(cspinner )
            return layout


    SpinnerApp().run()
