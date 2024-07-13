#pragma once

class menu : public frameObj, public box {
    private:
        std::vector<menuItem> items;
        std::string name;
        bool visable;
        FlexDirection direction;

    public:
        menu(point2 min_, point2 max_, point3 boxColor) : box(min_, max_, 1, boxColor), direction(ROW), frameObj(), visable(true) {}
        menu(point2 min_, point2 max_, point3 boxColor, std::vector<menuItem>& items) : box(min_, max_, 1, boxColor), items(items), direction(ROW), frameObj(), visable(true) {}
        menu(point2 min_, point2 max_, point3 boxColor, std::vector<menuItem> items, std::string name, FlexDirection direction, bool visable) : box(min_, max_, 1, boxColor, true), items(items), name(name), direction(direction), frameObj(), visable(visable) {}

        std::vector<menuItem> getItems() const { return this->items; }
        void addItem(menuItem& item) { this->items.emplace_back(item); }
        std::string getName() const { return this->name; }
        void setName(std::string& name) { this->name = name; }
        bool isVisable() const { return this->visable; }
        void setVisable(bool visable) { this->visable = visable; }
};
