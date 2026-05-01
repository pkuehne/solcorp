# Site Management

## Sites

A **site** is your "base". This is where you will house staff, construct rockets and launch them for money. It is represented as a grid of buildable tiles. Each tile can contain one building, and each building contains one or more facilities that provide capabilities (e.g. launchpads, offices, storage).

![Site](../assets/site.png "A site with a launch complex, office building and factory")

Sites are limited in size and the extent of the site is shown with a blue border. You can only build within the site boundary, but you can place buildings adjacent to the boundary.

## Buildings

Buildings are placed on site tiles at a grid position. Each building is of a particular type (e.g. "Launch Complex", "Office Building", "Factory").

Buildings contain one or more **facilities** that provide specific capabilities. Thus a building can have several launchpads, to launch more than one rocket at a time, or a combination of offices and storage. 

![Launchpad](../assets/launchpad.png "A launchpad building surrounded by construction markers")

Clicking on a building opens the building view, which shows the facilities contained within and allows you to manage operations related to those facilities.

Each building is surrounded by construction markers that indicate where new buildings may be placed. You can only build on tiles that have construction markers. When you place a building, construction markers will be added to adjacent tiles, allowing you to expand your site outwards. 

## Facilities

At present, there are four types of facility:

| Facility | Purpose |
|----------|---------|
| Launchpad | Required to execute a rocket launch |
| Office | Provides administrative capacity (not implemented) |
| Storage | Stores manufactured rockets |
| Manufacturing | Produces rockets |

![Building with two manufacturing facilities](../assets/building-with-two-facilities.png "Building with two manufacturing facilities")