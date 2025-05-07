# My best Qt GUI Practices

Note, this is highly opinionated and does not mean that it is the best, or correct way, however, this is the way that I believe things should be done when dealing with GUI programming with Qt. 

## Ownership

Ownership is king. 

Who is responsible for the object? 
Does this object need to be shared or can it be unique? 

One should always ask these questions and make sure that it is understood who owns what. 

## Inheritance

I typically try to avoid using inheritance if it can be done. But if it must be done try to make sure that it is only **ONE** layer deep. For example, a base class, `Car`, should only be inherited once. Polymorphism can get extremly messy and complicated if `RaceCar` inherits from `Car` and then a `McLaren_F1` inherits from `RaceCar`. Keep it simple. Just inherit from the `Car` and build upon that. 

Qt does not believe in this which is why it is important that we do not abuse the inheritance of an already deeply inherited heirarchy. Which is why I stress the importance of not repeating their same mistakes. Which is why something like this happens. 

In Qt, A `QWidget` does not "own" `QWidget`s. A `QWidget` can have a `QLayout` which the `QLayout` "owns" the `QWidgets`. 

## Object Management

Take the following example. I want to create a Panel that has three radio buttons on it to toggle. This is how I would construct it:

- Create a widget class that inherits from a `QWidget`
- Create a layout class that inherits from a `QLayout`
- Add the radio buttons to the layout class
- Add the layout to the widget class

Rinse and repeat. 

Is it messy? Yes. Is it clear? Yes. It keeps the two separated out and becomes much more cleaner to debug. Remember that ownership is king!